#include "pdf.h"
#include "src/ctx.h"

fz_page *render_cover(fz_context *ctx, document_t *doc, fz_document *fzdoc) {

    int err = 0;
    fz_page *cover = NULL;

    fz_var(cover);
    fz_try(ctx)
        cover = fz_load_page(ctx, fzdoc, 0);
    fz_catch(ctx)
        err = 1;

    if (err != 0) {
        fz_drop_page(ctx, cover);
        return NULL;
    }

    fz_rect bounds = fz_bound_page(ctx, cover);

    float scale;
    float w = (float) bounds.x1 - bounds.x0;
    float h = (float) bounds.y1 - bounds.y0;
    if (w > h) {
        scale = (float) ScanCtx.tn_size / w;
    } else {
        scale = (float) ScanCtx.tn_size / h;
    }
    fz_matrix m = fz_scale(scale, scale);

    bounds = fz_transform_rect(bounds, m);
    fz_irect bbox = fz_round_rect(bounds);
    fz_pixmap *pixmap = fz_new_pixmap_with_bbox(ctx, ctx->colorspace->rgb, bbox, NULL, 0);

    fz_clear_pixmap_with_value(ctx, pixmap, 0xFF);
    fz_device *dev = fz_new_draw_device(ctx, m, pixmap);

    fz_var(err);
    fz_try(ctx)
    {
        pthread_mutex_lock(&ScanCtx.mupdf_mu);
        fz_run_page(ctx, cover, dev, fz_identity, NULL);
    }
    fz_always(ctx)
    {
        fz_close_device(ctx, dev);
        fz_drop_device(ctx, dev);
        pthread_mutex_unlock(&ScanCtx.mupdf_mu);
    }
    fz_catch(ctx)
        err = ctx->error.errcode;

    if (err != 0) {
        fz_drop_page(ctx, cover);
        fz_drop_pixmap(ctx, pixmap);
        return NULL;
    }

    fz_buffer *fzbuf = NULL;
    fz_var(fzbuf);
    fz_var(err);

    fz_try(ctx)
        fzbuf = fz_new_buffer_from_pixmap_as_png(ctx, pixmap, fz_default_color_params);
    fz_catch(ctx)
        err = ctx->error.errcode;

    if (err == 0) {
        unsigned char *tn_buf;
        size_t tn_len = fz_buffer_storage(ctx, fzbuf, &tn_buf);
        store_write(ScanCtx.index.store, (char *) doc->uuid, sizeof(doc->uuid), (char *) tn_buf, tn_len);
    }

    fz_drop_buffer(ctx, fzbuf);
    fz_drop_pixmap(ctx, pixmap);

    if (err != 0) {
        fz_drop_page(ctx, cover);
        return NULL;
    }

    return cover;
}

void fz_noop_callback(__attribute__((unused)) void *user, __attribute__((unused)) const char *message) {}


void init_ctx(fz_context *ctx) {
    fz_disable_icc(ctx);
    fz_register_document_handlers(ctx);
    ctx->warn.print = fz_noop_callback;
    ctx->error.print = fz_noop_callback;
}

int read_stext_block(fz_stext_block *block, text_buffer_t *tex) {
    if (block->type != FZ_STEXT_BLOCK_TEXT) {
        return 0;
    }

    fz_stext_line *line = block->u.t.first_line;
    while (line != NULL) {
        fz_stext_char *c = line->first_char;
        while (c != NULL) {
            if (text_buffer_append_char(tex, c->c) == TEXT_BUF_FULL) {
                return TEXT_BUF_FULL;
            }
            c = c->next;
        }
        line = line->next;
    }
    return 0;
}

void parse_pdf(void *buf, size_t buf_len, document_t *doc) {

    if (buf == NULL) {
        return;
    }

    static int mu_is_initialized = 0;
    if (!mu_is_initialized) {
        pthread_mutex_init(&ScanCtx.mupdf_mu, NULL);
        mu_is_initialized = 1;
    }
    fz_context *ctx = fz_new_context(NULL, NULL, FZ_STORE_UNLIMITED);

    init_ctx(ctx);

    int err = 0;

    fz_document *fzdoc = NULL;
    fz_stream *stream = NULL;
    fz_var(fzdoc);
    fz_var(stream);
    fz_var(err);

    fz_try(ctx)
    {
        stream = fz_open_memory(ctx, buf, buf_len);
        fzdoc = fz_open_document_with_stream(ctx, mime_get_mime_text(doc->mime), stream);
    }
    fz_catch(ctx)
        err = ctx->error.errcode;

    if (err) {
        fz_drop_stream(ctx, stream);
        fz_drop_document(ctx, fzdoc);
        fz_drop_context(ctx);
        return;
    }

    char title[4096] = {'\0',};
    fz_try(ctx)
        fz_lookup_metadata(ctx, fzdoc, FZ_META_INFO_TITLE, title, sizeof(title));
    fz_catch(ctx)
        ;

    if (strlen(title) > 0) {
        meta_line_t *meta_content = malloc(sizeof(meta_line_t) + strlen(title));
        meta_content->key = MetaTitle;
        strcpy(meta_content->strval, title);
        APPEND_META(doc, meta_content)
    }

    int page_count = -1;
    fz_var(err);
    fz_try(ctx)
        page_count = fz_count_pages(ctx, fzdoc);
    fz_catch(ctx)
        err = ctx->error.errcode;

    if (err) {
        fz_drop_stream(ctx, stream);
        fz_drop_document(ctx, fzdoc);
        fz_drop_context(ctx);
        return;
    }

    fz_page *cover = NULL;
    if (ScanCtx.tn_size > 0) {
        cover = render_cover(ctx, doc, fzdoc);
    } else {
        fz_var(cover);
        fz_try(ctx)
            cover = fz_load_page(ctx, fzdoc, 0);
        fz_catch(ctx)
            cover = NULL;
    }

    if (cover == NULL) {
        fz_drop_stream(ctx, stream);
        fz_drop_document(ctx, fzdoc);
        fz_drop_context(ctx);
        return;
    }

    if (ScanCtx.content_size > 0) {
        fz_stext_options opts = {0};
        text_buffer_t text_buf = text_buffer_create(ScanCtx.content_size);

        for (int current_page = 0; current_page < page_count; current_page++) {
            fz_page *page = NULL;
            if (current_page == 0) {
                page = cover;
            } else {
                fz_var(err);
                fz_try(ctx)
                            page = fz_load_page(ctx, fzdoc, current_page);
                fz_catch(ctx)
                    err = ctx->error.errcode;
                if (err != 0) {
                    text_buffer_destroy(&text_buf);
                    fz_drop_page(ctx, page);
                    fz_drop_stream(ctx, stream);
                    fz_drop_document(ctx, fzdoc);
                    fz_drop_context(ctx);
                    return;
                }
            }

            fz_stext_page *stext = fz_new_stext_page(ctx, fz_bound_page(ctx, page));
            fz_device *dev = fz_new_stext_device(ctx, stext, &opts);

            fz_var(err);
            fz_try(ctx)
                        fz_run_page(ctx, page, dev, fz_identity, NULL);
            fz_always(ctx)
                {
                    fz_close_device(ctx, dev);
                    fz_drop_device(ctx, dev);
                }
            fz_catch(ctx)
                err = ctx->error.errcode;

            if (err != 0) {
                text_buffer_destroy(&text_buf);
                fz_drop_page(ctx, page);
                fz_drop_stext_page(ctx, stext);
                fz_drop_stream(ctx, stream);
                fz_drop_document(ctx, fzdoc);
                fz_drop_context(ctx);
                return;
            }

            fz_stext_block *block = stext->first_block;
            while (block != NULL) {
                int ret = read_stext_block(block, &text_buf);
                if (ret == TEXT_BUF_FULL) {
                    break;
                }
                block = block->next;
            }
            fz_drop_stext_page(ctx, stext);
            fz_drop_page(ctx, page);

            if (text_buf.dyn_buffer.cur >= text_buf.dyn_buffer.size) {
                break;
            }
        }
        text_buffer_terminate_string(&text_buf);

        meta_line_t *meta_content = malloc(sizeof(meta_line_t) + text_buf.dyn_buffer.cur);
        meta_content->key = MetaContent;
        memcpy(meta_content->strval, text_buf.dyn_buffer.buf, text_buf.dyn_buffer.cur);
        APPEND_META(doc, meta_content)

        text_buffer_destroy(&text_buf);
    }

    fz_drop_stream(ctx, stream);
    fz_drop_document(ctx, fzdoc);
    fz_drop_context(ctx);
}

