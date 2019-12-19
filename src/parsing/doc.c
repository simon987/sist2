#include "doc.h"

static void dumpText(mceTextReader_t *reader, dyn_buffer_t *buf) {

    mce_skip_attributes(reader);

    mce_start_children(reader) {
        mce_start_element(reader, NULL, _X("t")) {
            mce_skip_attributes(reader);
            mce_start_children(reader) {
                mce_start_text(reader) {
                    char *str = (char *) xmlTextReaderConstValue(reader->reader);
                    dyn_buffer_append_string(buf, str);
                    dyn_buffer_write_char(buf, ' ');
                } mce_end_text(reader);
            } mce_end_children(reader);
        } mce_end_element(reader);

        mce_start_element(reader, NULL, NULL) {
            dumpText(reader, buf);
        } mce_end_element(reader);

    } mce_end_children(reader)
}

__always_inline
int should_read_part(opcPart part) {

    char *part_name = (char *) part;

    if (part == NULL) {
        return FALSE;
    }

    if (    // Word
            strcmp(part_name, "word/document.xml") == 0
            || strncmp(part_name, "word/footer", sizeof("word/footer") - 1) == 0
            || strncmp(part_name, "word/header", sizeof("word/header") - 1) == 0
            // PowerPoint
            || strncmp(part_name, "ppt/slides/slide", sizeof("ppt/slides/slide") - 1) == 0
            || strncmp(part_name, "ppt/notesSlides/notesSlide", sizeof("ppt/notesSlides/notesSlide") - 1) == 0
            // Excel
            || strncmp(part_name, "xl/worksheets/sheet", sizeof("xl/worksheets/sheet") - 1) == 0
            || strcmp(part_name, "xl/sharedStrings.xml") == 0
            || strcmp(part_name, "xl/workbook.xml") == 0
            ) {
        return TRUE;
    }

    return FALSE;
}

void read_part(opcContainer *c, dyn_buffer_t *buf, opcPart part) {

    mceTextReader_t reader;
    int ret = opcXmlReaderOpen(c, &reader, part, NULL, "UTF-8", 0);

    if (ret != OPC_ERROR_NONE) {
        //todo verbose
        return;
    }

    mce_start_document(&reader) {
        mce_start_element(&reader, NULL, NULL) {
            dumpText(&reader, buf);
        } mce_end_element(&reader);
    }mce_end_document(&reader);

    mceTextReaderCleanup(&reader);
}

void parse_doc(void *mem, size_t mem_len, document_t *doc) {

    opcContainer *c = opcContainerOpenMem(mem, mem_len, OPC_OPEN_READ_ONLY, NULL);
    if (c == NULL) {
        //todo verbose
        return;
    }

    dyn_buffer_t buf = dyn_buffer_create();

    opcPart part = opcPartGetFirst(c);
    do {
        if (should_read_part(part)) {
            read_part(c, &buf, part);
        }
    } while ((part = opcPartGetNext(c, part)));

    opcContainerClose(c, OPC_CLOSE_NOW);
    dyn_buffer_write_char(&buf, '\0');

    meta_line_t *meta = malloc(sizeof(meta_line_t) + buf.cur);
    meta->key = MetaContent;
    strcpy(meta->strval, buf.buf);
    APPEND_META(doc, meta)

    dyn_buffer_destroy(&buf);
}
