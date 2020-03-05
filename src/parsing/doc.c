#include "doc.h"
#include "src/ctx.h"

__always_inline
static int should_read_part(char *part) {

    LOG_DEBUGF("doc.c", "Got part : %s", part)
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

typedef int (XMLCALL *xmlInputReadCallback)(void *context, char *buffer, int len);

typedef struct {
    struct archive *a;
} xml_io_ctx;

int xml_io_read(void *context, char *buffer, int len) {
    xml_io_ctx *ctx = context;

    //TODO: return value ?
    return archive_read_data(ctx->a, buffer, len);
}

int xml_io_close(void *context) {
    //noop
    return 0;
}

__always_inline
static int read_part(struct archive *a, dyn_buffer_t *buf, document_t *doc) {

    xmlNode *root, *first_child, *node1, *node2, *node3, *node4;

    xml_io_ctx ctx = {a};

    /* do actual parsing of document */
    xmlDoc *xml = xmlReadIO(xml_io_read, xml_io_close, &ctx, "/", NULL, 0);

    /* error checking! */
    if (xml == NULL) {
        fprintf(stderr, "Document not parsed successfully. \n");
        return -1;
    }
    root = xmlDocGetRootElement(xml);
    if (root == NULL) {
        fprintf(stderr, "empty document\n");
        xmlFreeDoc(xml);
        return -1;
    }
    if (xmlStrcmp(root->name, (const xmlChar *) "document") != 0) {
        fprintf(stderr, "document of the wrong type, root node != document");
        xmlFreeDoc(xml);
        return -1;
    }

    /* init a few more variables */
    xmlChar *key;

    first_child = root->children;
    for (node1 = first_child; node1; node1 = node1->next) {
        if ((xmlStrcmp(node1->name, (const xmlChar *) "body")) == 0) {
            for (node2 = node1->children; node2; node2 = node2->next) {
                if ((xmlStrcmp(node2->name, (const xmlChar *) "p")) == 0) {

                    dyn_buffer_write_char(buf, ' ');

                    for (node3 = node2->children; node3; node3 = node3->next) {
                        if ((xmlStrcmp(node3->name, (const xmlChar *) "r")) == 0) {
                            for (node4 = node3->children; node4; node4 = node4->next) {
                                if ((!xmlStrcmp(node4->name, (const xmlChar *) "t"))) {
                                    key = xmlNodeListGetString(xml, node4->xmlChildrenNode, 1);

                                    dyn_buffer_append_string(buf, (char *) key);
                                    dyn_buffer_write_char(buf, ' ');
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

void parse_doc(void *mem, size_t mem_len, document_t *doc) {

    if (mem == NULL) {
        return;
    }

    struct archive *a = archive_read_new();
    archive_read_support_format_zip(a);

    int ret = archive_read_open_memory(a, mem, mem_len);
    if (ret != ARCHIVE_OK) {
        LOG_ERRORF(doc->filepath, "Could not read archive: %s", archive_error_string(a));
        archive_read_free(a);
        return;
    }

    dyn_buffer_t buf = dyn_buffer_create();

    struct archive_entry *entry;
    while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
        if (S_ISREG(archive_entry_stat(entry)->st_mode)) {
            char *path = (char *) archive_entry_pathname(entry);

            if (should_read_part(path)) {
                ret = read_part(a, &buf, doc);
                if (ret != 0) {
                    break;
                }
            }

        }
    }


    // close

    if (buf.cur > 0) {
        dyn_buffer_write_char(&buf, '\0');

        meta_line_t *meta = malloc(sizeof(meta_line_t) + buf.cur);
        meta->key = MetaContent;
        strcpy(meta->strval, buf.buf);
        APPEND_META(doc, meta)
    }

    dyn_buffer_destroy(&buf);
}
