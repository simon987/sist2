#include "pdf.h"
#include "src/ctx.h"

__always_inline
fz_page *render_cover(fz_context *ctx, document_t *doc, fz_document *fzdoc) {

    fz_page *cover = fz_load_page(ctx, fzdoc, 0);
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

    pthread_mutex_lock(&ScanCtx.mupdf_mu);
    fz_run_page(ctx, cover, dev, fz_identity, NULL);
    pthread_mutex_unlock(&ScanCtx.mupdf_mu);

    fz_drop_device(ctx, dev);

    fz_buffer *fzbuf = fz_new_buffer_from_pixmap_as_png(ctx, pixmap, fz_default_color_params);
    unsigned char *tn_buf;
    size_t tn_len = fz_buffer_storage(ctx, fzbuf, &tn_buf);

    store_write(ScanCtx.index.store, (char *) doc->uuid, sizeof(doc->uuid), (char *) tn_buf, tn_len);

    fz_drop_pixmap(ctx, pixmap);
    fz_drop_buffer(ctx, fzbuf);

    return cover;
}

void fz_noop_callback(__attribute__((unused)) void *user, __attribute__((unused)) const char *message) {}


void parse_pdf(void *buf, size_t buf_len, document_t *doc) {

    static int mu_is_initialized = 0;
    if (!mu_is_initialized) {
        pthread_mutex_init(&ScanCtx.mupdf_mu, NULL);
        mu_is_initialized = 1;
    }
    fz_context *ctx = fz_new_context(NULL, NULL, FZ_STORE_UNLIMITED);
    fz_stream *stream = NULL;
    fz_document *fzdoc = NULL;

    fz_var(stream);
    fz_var(fzdoc);

    fz_try(ctx)
    {
        fz_disable_icc(ctx);
        fz_register_document_handlers(ctx);

        //disable warnings
        ctx->warn.print = fz_noop_callback;
        ctx->error.print = fz_noop_callback;

        stream = fz_open_memory(ctx, buf, buf_len);
        fzdoc = fz_open_document_with_stream(ctx, mime_get_mime_text(doc->mime), stream);

        int page_count = fz_count_pages(ctx, fzdoc);
        fz_page *cover = render_cover(ctx, doc, fzdoc);

        fz_stext_options opts;

        text_buffer_t text_buf = text_buffer_create(ScanCtx.content_size);

        for (int current_page = 0; current_page < page_count; current_page++) {
            fz_page *page;
            if (current_page == 0) {
                page = cover;
            } else {
                page = fz_load_page(ctx, fzdoc, current_page);
            }

            fz_stext_page *stext = fz_new_stext_page(ctx, fz_bound_page(ctx, page));
            fz_device *dev = fz_new_stext_device(ctx, stext, &opts);

            pthread_mutex_lock(&ScanCtx.mupdf_mu);
            fz_run_page_contents(ctx, page, dev, fz_identity, NULL);
            pthread_mutex_unlock(&ScanCtx.mupdf_mu);

            fz_drop_device(ctx, dev);

            fz_stext_block *block = stext->first_block;
            while (block != NULL) {

                if (block->type != FZ_STEXT_BLOCK_TEXT) {
                    block = block->next;
                    continue;
                }

                fz_stext_line *line = block->u.t.first_line;
                while (line != NULL) {
                    fz_stext_char *c = line->first_char;
                    while (c != NULL) {
                        if (text_buffer_append_char(&text_buf, c->c) == TEXT_BUF_FULL) {
                            fz_drop_page(ctx, page);
                            fz_drop_stext_page(ctx, stext);
                            goto write_loop_end;
                        }
                        c = c->next;
                    }
                    line = line->next;
                }
                block = block->next;
            }
            fz_drop_page(ctx, page);
            fz_drop_stext_page(ctx, stext);
        }
        write_loop_end:;
        text_buffer_terminate_string(&text_buf);

        meta_line_t *meta_content = malloc(sizeof(meta_line_t) + text_buf.dyn_buffer.cur);
        meta_content->key = MetaContent;
        memcpy(meta_content->strval, text_buf.dyn_buffer.buf, text_buf.dyn_buffer.cur);
        text_buffer_destroy(&text_buf);
        APPEND_META(doc, meta_content)
    }
    fz_always(ctx)
    {
        fz_drop_stream(ctx, stream);
        fz_drop_document(ctx, fzdoc);
        fz_drop_context(ctx);
    } fz_catch(ctx) {
        fprintf(stderr, "Error %s %s\n", doc->filepath, ctx->error.message);
    }
}

