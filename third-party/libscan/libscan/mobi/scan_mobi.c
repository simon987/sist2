#include "scan_mobi.h"

#include <mobi.h>
#include <errno.h>
#include "stdlib.h"

void parse_mobi(scan_mobi_ctx_t *ctx, vfile_t *f, document_t *doc) {

    MOBIData *m = mobi_init();
    if (m == NULL) {
        CTX_LOG_ERROR(f->filepath, "mobi_init() failed")
        return;
    }

    size_t buf_len;
    char* buf = read_all(f, &buf_len);
    if (buf == NULL) {
        mobi_free(m);
        CTX_LOG_ERROR(f->filepath, "read_all() failed")
        return;
    }

    FILE *file = fmemopen(buf, buf_len, "rb");
    if (file == NULL) {
        mobi_free(m);
        free(buf);
        CTX_LOG_ERRORF(f->filepath, "fmemopen() failed (%d)", errno)
        return;
    }

    MOBI_RET mobi_ret = mobi_load_file(m, file);
    fclose(file);
    if (mobi_ret != MOBI_SUCCESS) {
        mobi_free(m);
        free(buf);
        CTX_LOG_ERRORF(f->filepath, "mobi_laod_file() returned error code [%d]", mobi_ret)
        return;
    }

    char *author = mobi_meta_get_author(m);
    if (author != NULL) {
        APPEND_STR_META(doc, MetaAuthor, author)
        free(author);
    }
    char *title = mobi_meta_get_title(m);
    if (title != NULL) {
        APPEND_STR_META(doc, MetaTitle, title)
        free(title);
    }

    const size_t maxlen = mobi_get_text_maxsize(m);
    if (maxlen == MOBI_NOTSET) {
        free(buf);
        CTX_LOG_DEBUGF("%s", "Invalid text maxsize: %zu", maxlen)
        return;
    }

    char *content_str = malloc(maxlen + 1);
    size_t length = maxlen;
    mobi_ret = mobi_get_rawml(m, content_str, &length);
    if (mobi_ret != MOBI_SUCCESS) {
        mobi_free(m);
        free(content_str);
        free(buf);
        CTX_LOG_ERRORF(f->filepath, "mobi_get_rawml() returned error code [%d]", mobi_ret)
        return;
    }

    text_buffer_t tex = text_buffer_create(ctx->content_size);
    text_buffer_append_markup(&tex, content_str);
    text_buffer_terminate_string(&tex);

    APPEND_STR_META(doc, MetaContent, tex.dyn_buffer.buf)

    free(content_str);
    free(buf);
    text_buffer_destroy(&tex);
    mobi_free(m);
}