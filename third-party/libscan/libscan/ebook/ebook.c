#include "ebook.h"
#include "../util.h"
#include <mupdf/fitz.h>
#include <pthread.h>
#include <tesseract/capi.h>

#define MIN_OCR_SIZE 350
#define MIN_OCR_LEN 10

/* fill_image callback doesn't let us pass opaque pointers unless I create my own device */
__thread text_buffer_t thread_buffer;
__thread scan_ebook_ctx_t thread_ctx;


int render_cover(scan_ebook_ctx_t *ctx, fz_context *fzctx, document_t *doc, fz_document *fzdoc) {

    int err = 0;
    fz_page *cover = NULL;

    fz_var(cover);
    fz_var(err);
    fz_try(fzctx)
        cover = fz_load_page(fzctx, fzdoc, 0);
    fz_catch(fzctx)
        err = 1;

    if (err != 0) {
        fz_drop_page(fzctx, cover);
//        LOG_WARNINGF(doc->filepath, "fz_load_page() returned error code [%d] %s", err, ctx->error.message)
        return FALSE;
    }

    fz_rect bounds = fz_bound_page(fzctx, cover);

    float scale;
    float w = (float) bounds.x1 - bounds.x0;
    float h = (float) bounds.y1 - bounds.y0;
    if (w > h) {
        scale = (float) ctx->tn_size / w;
    } else {
        scale = (float) ctx->tn_size / h;
    }
    fz_matrix m = fz_scale(scale, scale);

    bounds = fz_transform_rect(bounds, m);
    fz_irect bbox = fz_round_rect(bounds);
    fz_pixmap *pixmap = fz_new_pixmap_with_bbox(fzctx, fzctx->colorspace->rgb, bbox, NULL, 0);

    fz_clear_pixmap_with_value(fzctx, pixmap, 0xFF);
    fz_device *dev = fz_new_draw_device(fzctx, m, pixmap);

    fz_var(err);
    fz_try(fzctx)
    {
        pthread_mutex_lock(&ctx->mupdf_mutex);
        fz_run_page(fzctx, cover, dev, fz_identity, NULL);
    }
    fz_always(fzctx)
    {
        fz_close_device(fzctx, dev);
        fz_drop_device(fzctx, dev);
        pthread_mutex_unlock(&ctx->mupdf_mutex);
    }
    fz_catch(fzctx)
        err = fzctx->error.errcode;

    if (err != 0) {
//        LOG_WARNINGF(doc->filepath, "fz_run_page() returned error code [%d] %s", err, ctx->error.message)
        fz_drop_page(fzctx, cover);
        fz_drop_pixmap(fzctx, pixmap);
        return FALSE;
    }

    fz_buffer *fzbuf = NULL;
    fz_var(fzbuf);
    fz_var(err);

    fz_try(fzctx)
        fzbuf = fz_new_buffer_from_pixmap_as_png(fzctx, pixmap, fz_default_color_params);
    fz_catch(fzctx)
        err = fzctx->error.errcode;

    if (err == 0) {
        unsigned char *tn_buf;
        size_t tn_len = fz_buffer_storage(fzctx, fzbuf, &tn_buf);
//        store_write(ScanCtx.index.store, (char *) doc->uuid, sizeof(doc->uuid), (char *) tn_buf, tn_len);
    }

    fz_drop_buffer(fzctx, fzbuf);
    fz_drop_pixmap(fzctx, pixmap);
    fz_drop_page(fzctx, cover);

    if (err != 0) {
//        LOG_WARNINGF(doc->filepath, "fz_new_buffer_from_pixmap_as_png() returned error code [%d] %s", err,
//                     ctx->error.message)
        return FALSE;
    }

    return TRUE;
}

void fz_err_callback(void *user, UNUSED(const char *message)) {
//    if (LogCtx.verbose) {
//        document_t *doc = (document_t *) user;
//        LOG_WARNINGF(doc->filepath, "FZ: %s", message)
//    }
}

static void init_fzctx(fz_context *fzctx, document_t *doc) {
    fz_disable_icc(fzctx);
    fz_register_document_handlers(fzctx);

    fzctx->warn.print_user = doc;
    fzctx->warn.print = fz_err_callback;
    fzctx->error.print_user = doc;
    fzctx->error.print = fz_err_callback;
}

static int read_stext_block(fz_stext_block *block, text_buffer_t *tex) {
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

#define IS_VALID_BPP(d) (d==1 || d==2 || d==4 || d==8 || d==16 || d==24 || d==32)

void fill_image(fz_context *fzctx, UNUSED(fz_device *dev),
                fz_image *img, UNUSED(fz_matrix ctm), UNUSED(float alpha),
                UNUSED(fz_color_params color_params)) {

    int l2factor = 0;

    if (img->w > MIN_OCR_SIZE && img->h > MIN_OCR_SIZE && IS_VALID_BPP(img->n)) {

        fz_pixmap *pix = img->get_pixmap(fzctx, img, NULL, img->w, img->h, &l2factor);

        if (pix->h > MIN_OCR_SIZE && img->h > MIN_OCR_SIZE && img->xres != 0) {
            TessBaseAPI *api = TessBaseAPICreate();
            TessBaseAPIInit3(api, thread_ctx.tesseract_path, thread_ctx.tesseract_lang);

            TessBaseAPISetImage(api, pix->samples, pix->w, pix->h, pix->n, pix->stride);
            TessBaseAPISetSourceResolution(api, pix->xres);

            char *text = TessBaseAPIGetUTF8Text(api);
            size_t len = strlen(text);
            if (len >= MIN_OCR_LEN) {
                text_buffer_append_string(&thread_buffer, text, len - 1);
//                LOG_DEBUGF(
//                        "ebook.c",
//                        "(OCR) %dx%d got %dB from tesseract (%s), buffer:%dB",
//                        pix->w, pix->h, len, ScanCtx.tesseract_lang, thread_buffer.dyn_buffer.cur
//                )
            }

            TessBaseAPIEnd(api);
            TessBaseAPIDelete(api);
        }
        fz_drop_pixmap(fzctx, pix);
    }
}

void parse_ebook(scan_ebook_ctx_t *ctx, vfile_t *f, const char* mime_str,  document_t *doc) {

    size_t buf_len;
    void * buf = read_all(f, &buf_len);

    static int mu_is_initialized = 0;
    if (!mu_is_initialized) {
        pthread_mutex_init(&ctx->mupdf_mutex, NULL);
        mu_is_initialized = 1;
    }
    fz_context *fzctx = fz_new_context(NULL, NULL, FZ_STORE_UNLIMITED);

    init_fzctx(fzctx, doc);

    int err = 0;

    fz_document *fzdoc = NULL;
    fz_stream *stream = NULL;
    fz_var(fzdoc);
    fz_var(stream);
    fz_var(err);

    fz_try(fzctx)
    {
        stream = fz_open_memory(fzctx, buf, buf_len);
        fzdoc = fz_open_document_with_stream(fzctx, mime_str, stream);
    }
    fz_catch(fzctx)
        err = fzctx->error.errcode;

    if (err != 0) {
        fz_drop_stream(fzctx, stream);
        fz_drop_document(fzctx, fzdoc);
        fz_drop_context(fzctx);
        return;
    }

    char title[4096] = {'\0',};
    fz_try(fzctx)
        fz_lookup_metadata(fzctx, fzdoc, FZ_META_INFO_TITLE, title, sizeof(title));
    fz_catch(fzctx)
        ;

    if (strlen(title) > 0) {
        meta_line_t *meta_content = malloc(sizeof(meta_line_t) + strlen(title));
        meta_content->key = MetaTitle;
        strcpy(meta_content->str_val, title);
        APPEND_META(doc, meta_content)
    }

    int page_count = -1;
    fz_var(err);
    fz_try(fzctx)
        page_count = fz_count_pages(fzctx, fzdoc);
    fz_catch(fzctx)
        err = fzctx->error.errcode;

    if (err) {
//        LOG_WARNINGF(doc->filepath, "fz_count_pages() returned error code [%d] %s", err, ctx->error.message)
        fz_drop_stream(fzctx, stream);
        fz_drop_document(fzctx, fzdoc);
        fz_drop_context(fzctx);
        return;
    }

    if (ctx->tn_size > 0) {
        err = render_cover(ctx, fzctx, doc, fzdoc);
    }

    if (err == TRUE) {
        fz_drop_stream(fzctx, stream);
        fz_drop_document(fzctx, fzdoc);
        fz_drop_context(fzctx);
        return;
    }

    if (ctx->content_size > 0) {
        fz_stext_options opts = {0};
        thread_buffer = text_buffer_create(ctx->content_size);

        for (int current_page = 0; current_page < page_count; current_page++) {
            fz_page *page = NULL;
            fz_var(err);
            fz_try(fzctx)
                page = fz_load_page(fzctx, fzdoc, current_page);
            fz_catch(fzctx)
                err = fzctx->error.errcode;
            if (err != 0) {
//                LOG_WARNINGF(doc->filepath, "fz_load_page() returned error code [%d] %s", err, ctx->error.message)
                text_buffer_destroy(&thread_buffer);
                fz_drop_page(fzctx, page);
                fz_drop_stream(fzctx, stream);
                fz_drop_document(fzctx, fzdoc);
                fz_drop_context(fzctx);
                return;
            }

            fz_stext_page *stext = fz_new_stext_page(fzctx, fz_bound_page(fzctx, page));
            fz_device *dev = fz_new_stext_device(fzctx, stext, &opts);
            dev->stroke_path = NULL;
            dev->stroke_text = NULL;
            dev->clip_text = NULL;
            dev->clip_stroke_path = NULL;
            dev->clip_stroke_text = NULL;

            if (ctx->tesseract_lang!= NULL) {
                dev->fill_image = fill_image;
            }

            fz_var(err);
            fz_try(fzctx)
                fz_run_page(fzctx, page, dev, fz_identity, NULL);
            fz_always(fzctx)
            {
                fz_close_device(fzctx, dev);
                fz_drop_device(fzctx, dev);
            }
            fz_catch(fzctx)
                err = fzctx->error.errcode;

            if (err != 0) {
//                LOG_WARNINGF(doc->filepath, "fz_run_page() returned error code [%d] %s", err, ctx->error.message)
                text_buffer_destroy(&thread_buffer);
                fz_drop_page(fzctx, page);
                fz_drop_stext_page(fzctx, stext);
                fz_drop_stream(fzctx, stream);
                fz_drop_document(fzctx, fzdoc);
                fz_drop_context(fzctx);
                return;
            }

            fz_stext_block *block = stext->first_block;
            while (block != NULL) {
                int ret = read_stext_block(block, &thread_buffer);
                if (ret == TEXT_BUF_FULL) {
                    break;
                }
                block = block->next;
            }
            fz_drop_stext_page(fzctx, stext);
            fz_drop_page(fzctx, page);

            if (thread_buffer.dyn_buffer.cur >= thread_buffer.dyn_buffer.size) {
                break;
            }
        }
        text_buffer_terminate_string(&thread_buffer);

        meta_line_t *meta_content = malloc(sizeof(meta_line_t) + thread_buffer.dyn_buffer.cur);
        meta_content->key = MetaContent;
        memcpy(meta_content->str_val, thread_buffer.dyn_buffer.buf, thread_buffer.dyn_buffer.cur);
        APPEND_META(doc, meta_content)

        text_buffer_destroy(&thread_buffer);
    }

    fz_drop_stream(fzctx, stream);
    fz_drop_document(fzctx, fzdoc);
    fz_drop_context(fzctx);
}
