#include "text.h"
#include "src/ctx.h"

void parse_text(int bytes_read, int *fd, char *buf, document_t *doc) {

    char *intermediate_buf;
    int intermediate_buf_len;

    if (bytes_read == doc->size || bytes_read >= ScanCtx.content_size) {
        int to_copy = MIN(bytes_read, ScanCtx.content_size);
        intermediate_buf = malloc(to_copy);
        intermediate_buf_len = to_copy;
        memcpy(intermediate_buf, buf, to_copy);

    } else {
        if (*fd == -1) {
            *fd = open(doc->filepath, O_RDONLY);
        }

        int to_read = MIN(ScanCtx.content_size, doc->size) - bytes_read;

        intermediate_buf = malloc(to_read + bytes_read);
        intermediate_buf_len = to_read + bytes_read;
        if (bytes_read != 0) {
            memcpy(intermediate_buf, buf, bytes_read);
        }

        read(*fd, intermediate_buf + bytes_read, to_read);
    }

    text_buffer_t text_buf = text_buffer_create(ScanCtx.content_size);
    for (int i = 0; i < intermediate_buf_len; i++) {
        text_buffer_append_char(&text_buf, *(intermediate_buf + i));
    }
    text_buffer_terminate_string(&text_buf);

    meta_line_t *meta = malloc(sizeof(meta_line_t) + text_buf.dyn_buffer.cur);
    meta->key = MetaContent;
    strcpy(meta->strval, text_buf.dyn_buffer.buf);
    text_buffer_destroy(&text_buf);
    free(intermediate_buf);
    APPEND_META(doc, meta)
}
