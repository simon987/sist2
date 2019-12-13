#include "text.h"
#include "src/ctx.h"

void parse_text(int bytes_read, struct vfile *f, char *buf, document_t *doc) {

    char *intermediate_buf;
    int intermediate_buf_len;

    if (bytes_read == doc->size || bytes_read >= ScanCtx.content_size) {
        int to_copy = MIN(bytes_read, ScanCtx.content_size);
        intermediate_buf = malloc(to_copy);
        intermediate_buf_len = to_copy;
        memcpy(intermediate_buf, buf, to_copy);

    } else {
        int to_read = MIN(ScanCtx.content_size, doc->size) - bytes_read;

        intermediate_buf = malloc(to_read + bytes_read);
        intermediate_buf_len = to_read + bytes_read;
        if (bytes_read != 0) {
            memcpy(intermediate_buf, buf, bytes_read);
        }

        f->read(f, intermediate_buf + bytes_read, to_read);
    }
    text_buffer_t tex = text_buffer_create(ScanCtx.content_size);
    text_buffer_append_string(&tex, intermediate_buf, intermediate_buf_len);

    meta_line_t *meta = malloc(sizeof(meta_line_t) + tex.dyn_buffer.cur);
    meta->key = MetaContent;
    strcpy(meta->strval, tex.dyn_buffer.buf);
    APPEND_META(doc, meta)

    free(intermediate_buf);
    text_buffer_destroy(&tex);
}
