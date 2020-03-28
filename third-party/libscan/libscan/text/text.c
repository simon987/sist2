#include "text.h"

scan_code_t parse_text(scan_text_ctx_t *ctx, struct vfile *f, document_t *doc) {

    int to_read = MIN(ctx->content_size, doc->size);

    char *buf = malloc(to_read);
    int ret = f->read(f, buf, to_read);
    if (ret < 0) {
        //TODO: log
        return SCAN_ERR_READ;
    }

    text_buffer_t tex = text_buffer_create(ctx->content_size);
    text_buffer_append_string(&tex, buf, to_read);
    text_buffer_terminate_string(&tex);

    meta_line_t *meta = malloc(sizeof(meta_line_t) + tex.dyn_buffer.cur);
    meta->key = MetaContent;
    strcpy(meta->str_val, tex.dyn_buffer.buf);

    APPEND_META(doc, meta)

    printf("%s", meta->str_val);

    free(buf);
    text_buffer_destroy(&tex);

    return SCAN_OK;
}

