#include "ebook.h"
#include <mupdf/fitz.h>
#include <pthread.h>
#include <tesseract/capi.h>

#include "../media/media.h"
#include "../arc/arc.h"

#define MIN_OCR_SIZE 350
#define MIN_OCR_LEN 10

/* fill_image callback doesn't let us pass opaque pointers unless I create my own device */
__thread text_buffer_t thread_buffer;
__thread scan_ebook_ctx_t thread_ctx;

pthread_mutex_t Mutex;

static void my_fz_lock(UNUSED(void *user), int lock) {
    if (lock == FZ_LOCK_FREETYPE) {
        pthread_mutex_lock(&Mutex);
    }
}

static void my_fz_unlock(UNUSED(void *user), int lock) {
    if (lock == FZ_LOCK_FREETYPE) {
        pthread_mutex_unlock(&Mutex);
    }
}


int pixmap_is_blank(const fz_pixmap *pixmap) {
    int pixmap_size = pixmap->n * pixmap->w * pixmap->h;
    const int pixel0 = pixmap->samples[0];
    for (int i = 0; i < pixmap_size; i++) {
        if (pixmap->samples[i] != pixel0) {
            return FALSE;
        }
    }
    return TRUE;
}

fz_pixmap *
load_pixmap(scan_ebook_ctx_t *ctx, int page, fz_context *fzctx, fz_document *fzdoc, document_t *doc, fz_page **cover) {

    int err = 0;

    fz_var(cover);
    fz_var(err);
    fz_try(fzctx)*cover = fz_load_page(fzctx, fzdoc, page);
    fz_catch(fzctx)err = 1;

    if (err != 0) {
        CTX_LOG_WARNINGF(doc->filepath, "fz_load_page() returned error code [%d] %s", err, fzctx->error.message)
        return NULL;
    }

    fz_rect bounds = fz_bound_page(fzctx, *cover);

    float scale;
    float w = bounds.x1 - bounds.x0;
    float h = bounds.y1 - bounds.y0;
    if (w > h) {
        scale = (float) ctx->tn_size / w;
    } else {
        scale = (float) ctx->tn_size / h;
    }
    fz_matrix m = fz_scale(scale, scale);

    bounds = fz_transform_rect(bounds, m);
    fz_irect bbox = fz_round_rect(bounds);
    fz_pixmap *pixmap = fz_new_pixmap_with_bbox(fzctx, fz_device_rgb(fzctx), bbox, NULL, 0);

    fz_clear_pixmap_with_value(fzctx, pixmap, 0xFF);
    fz_device *dev = fz_new_draw_device(fzctx, m, pixmap);

    fz_var(err);
    fz_try(fzctx) {
                fz_run_page(fzctx, *cover, dev, fz_identity, NULL);
            } fz_always(fzctx) {
            fz_close_device(fzctx, dev);
            fz_drop_device(fzctx, dev);
        } fz_catch(fzctx)err = fzctx->error.errcode;

    if (err != 0) {
        CTX_LOG_WARNINGF(doc->filepath, "fz_run_page() returned error code [%d] %s", err, fzctx->error.message)
        fz_drop_page(fzctx, *cover);
        fz_drop_pixmap(fzctx, pixmap);
        return NULL;
    }

    if (pixmap->n != 3) {
        CTX_LOG_ERRORF(doc->filepath, "Got unexpected pixmap depth: %d", pixmap->n)
        fz_drop_page(fzctx, *cover);
        fz_drop_pixmap(fzctx, pixmap);
        return NULL;
    }

    return pixmap;
}

int render_cover(scan_ebook_ctx_t *ctx, fz_context *fzctx, document_t *doc, fz_document *fzdoc) {

    fz_page *cover = NULL;
    fz_pixmap *pixmap = load_pixmap(ctx, 0, fzctx, fzdoc, doc, &cover);
    if (pixmap == NULL) {
        return FALSE;
    }

    if (pixmap_is_blank(pixmap)) {
        fz_drop_page(fzctx, cover);
        fz_drop_pixmap(fzctx, pixmap);
        CTX_LOG_DEBUG(doc->filepath, "Cover page is blank, using page 1 instead")
        pixmap = load_pixmap(ctx, 1, fzctx, fzdoc, doc, &cover);
        if (pixmap == NULL) {
            return FALSE;
        }
    }

    // RGB24 -> YUV420p
    AVFrame *scaled_frame = av_frame_alloc();

    struct SwsContext *sws_ctx = sws_getContext(
            pixmap->w, pixmap->h, AV_PIX_FMT_RGB24,
            pixmap->w, pixmap->h, AV_PIX_FMT_YUV420P,
            SIST_SWS_ALGO, 0, 0, 0
    );

    int dst_buf_len = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, pixmap->w, pixmap->h, 1);
    uint8_t *dst_buf = (uint8_t *) av_malloc(dst_buf_len);

    av_image_fill_arrays(scaled_frame->data, scaled_frame->linesize, dst_buf, AV_PIX_FMT_YUV420P, pixmap->w, pixmap->h,
                         1);

    unsigned char *samples = calloc(1, 1024 * 1024 * 1024);
    memcpy(samples, pixmap->samples, pixmap->stride * pixmap->h);

    const uint8_t *in_data[1] = {samples,};
    int in_line_size[1] = {(int) pixmap->stride};

    sws_scale(sws_ctx,
              in_data, in_line_size,
              0, pixmap->h,
              scaled_frame->data, scaled_frame->linesize
    );

    scaled_frame->width = pixmap->w;
    scaled_frame->height = pixmap->h;
    scaled_frame->format = AV_PIX_FMT_YUV420P;

    sws_freeContext(sws_ctx);

    // YUV420p -> JPEG
    AVCodecContext *jpeg_encoder = alloc_jpeg_encoder(pixmap->w, pixmap->h, ctx->tn_qscale);
    avcodec_send_frame(jpeg_encoder, scaled_frame);

    AVPacket jpeg_packet;
    av_init_packet(&jpeg_packet);
    avcodec_receive_packet(jpeg_encoder, &jpeg_packet);

    APPEND_TN_META(doc, pixmap->w, pixmap->h)
    ctx->store((char *) doc->path_md5, sizeof(doc->path_md5), (char *) jpeg_packet.data, jpeg_packet.size);

    free(samples);
    av_packet_unref(&jpeg_packet);
    av_free(*scaled_frame->data);
    av_frame_free(&scaled_frame);
    avcodec_free_context(&jpeg_encoder);

    fz_drop_pixmap(fzctx, pixmap);
    fz_drop_page(fzctx, cover);

    return TRUE;
}

void fz_err_callback(void *user, const char *message) {
    document_t *doc = (document_t *) user;

    const scan_ebook_ctx_t *ctx = &thread_ctx;
    CTX_LOG_WARNINGF(doc->filepath, "FZ: %s", message)
}

void fz_warn_callback(void *user, const char *message) {
    document_t *doc = (document_t *) user;

    const scan_ebook_ctx_t *ctx = &thread_ctx;
    CTX_LOG_DEBUGF(doc->filepath, "FZ: %s", message)
}

static void init_fzctx(fz_context *fzctx, document_t *doc) {
    fz_register_document_handlers(fzctx);

    static int mu_is_initialized = FALSE;
    if (!mu_is_initialized) {
        pthread_mutex_init(&Mutex, NULL);
        mu_is_initialized = TRUE;
    }

    fzctx->warn.print_user = doc;
    fzctx->warn.print = fz_warn_callback;
    fzctx->error.print_user = doc;
    fzctx->error.print = fz_err_callback;

    fzctx->locks.lock = my_fz_lock;
    fzctx->locks.unlock = my_fz_unlock;
}

static int read_stext_block(fz_stext_block *block, text_buffer_t *tex) {
    if (block->type != FZ_STEXT_BLOCK_TEXT) {
        return 0;
    }

    fz_stext_line *line = block->u.t.first_line;
    while (line != NULL) {
        text_buffer_append_char(tex, ' ');
        fz_stext_char *c = line->first_char;
        while (c != NULL) {
            if (text_buffer_append_char(tex, c->c) == TEXT_BUF_FULL) {
                return TEXT_BUF_FULL;
            }
            c = c->next;
        }
        line = line->next;
    }
    text_buffer_append_char(tex, ' ');
    return 0;
}

#define IS_VALID_BPP(d) ((d)==1 || (d)==2 || (d)==4 || (d)==8 || (d)==16 || (d)==24 || (d)==32)

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
            }

            TessBaseAPIEnd(api);
            TessBaseAPIDelete(api);
        }
        fz_drop_pixmap(fzctx, pix);
    }
}

void
parse_ebook_mem(scan_ebook_ctx_t *ctx, void *buf, size_t buf_len, const char *mime_str, document_t *doc, int tn_only) {

    fz_context *fzctx = fz_new_context(NULL, NULL, FZ_STORE_DEFAULT);
    thread_ctx = *ctx;

    init_fzctx(fzctx, doc);

    int err = 0;

    fz_document *fzdoc = NULL;
    fz_stream *stream = NULL;
    fz_var(fzdoc);
    fz_var(stream);
    fz_var(err);

    fz_try(fzctx) {
                stream = fz_open_memory(fzctx, buf, buf_len);
                fzdoc = fz_open_document_with_stream(fzctx, mime_str, stream);
            } fz_catch(fzctx)err = fzctx->error.errcode;

    if (err != 0) {
        fz_drop_stream(fzctx, stream);
        fz_drop_document(fzctx, fzdoc);
        fz_drop_context(fzctx);
        return;
    }

    int page_count = -1;
    fz_var(err);
    fz_try(fzctx)page_count = fz_count_pages(fzctx, fzdoc);
    fz_catch(fzctx)err = fzctx->error.errcode;

    if (err) {
        CTX_LOG_WARNINGF(doc->filepath, "fz_count_pages() returned error code [%d] %s", err, fzctx->error.message)
        fz_drop_stream(fzctx, stream);
        fz_drop_document(fzctx, fzdoc);
        fz_drop_context(fzctx);
        return;
    }

    APPEND_LONG_META(doc, MetaPages, page_count)

    if (ctx->tn_size > 0) {
        if (render_cover(ctx, fzctx, doc, fzdoc) == FALSE) {
            fz_drop_stream(fzctx, stream);
            fz_drop_document(fzctx, fzdoc);
            fz_drop_context(fzctx);
            return;
        }
    }

    if (tn_only) {
        fz_drop_stream(fzctx, stream);
        fz_drop_document(fzctx, fzdoc);
        fz_drop_context(fzctx);
        return;
    }

    char title[8192] = {'\0',};
    fz_try(fzctx)fz_lookup_metadata(fzctx, fzdoc, FZ_META_INFO_TITLE, title, sizeof(title));
    fz_catch(fzctx);

    if (strlen(title) > 0) {
        APPEND_UTF8_META(doc, MetaTitle, title)
    }

    char author[4096] = {'\0',};
    fz_try(fzctx)fz_lookup_metadata(fzctx, fzdoc, FZ_META_INFO_AUTHOR, author, sizeof(author));
    fz_catch(fzctx);

    if (strlen(author) > 0) {
        APPEND_UTF8_META(doc, MetaAuthor, author)
    }


    if (ctx->content_size > 0) {
        fz_stext_options opts = {0};
        thread_buffer = text_buffer_create(ctx->content_size);

        for (int current_page = 0; current_page < page_count; current_page++) {
            fz_page *page = NULL;
            fz_var(err);
            fz_try(fzctx)page = fz_load_page(fzctx, fzdoc, current_page);
            fz_catch(fzctx)err = fzctx->error.errcode;
            if (err != 0) {
                CTX_LOG_WARNINGF(doc->filepath, "fz_load_page() returned error code [%d] %s", err, fzctx->error.message)
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

            if (ctx->tesseract_lang != NULL) {
                dev->fill_image = fill_image;
            }

            fz_var(err);
            fz_try(fzctx)fz_run_page(fzctx, page, dev, fz_identity, NULL);
            fz_always(fzctx) {
                    fz_close_device(fzctx, dev);
                    fz_drop_device(fzctx, dev);
                } fz_catch(fzctx)err = fzctx->error.errcode;

            if (err != 0) {
                CTX_LOG_WARNINGF(doc->filepath, "fz_run_page() returned error code [%d] %s", err, fzctx->error.message)
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

            if (thread_buffer.dyn_buffer.cur >= ctx->content_size) {
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

static scan_arc_ctx_t arc_ctx = (scan_arc_ctx_t) {.passphrase = {0,}};

void parse_epub_fast(scan_ebook_ctx_t *ctx, vfile_t *f, document_t *doc) {
    struct archive *a = NULL;
    struct archive_entry *entry = NULL;
    arc_data_t arc_data;

    text_buffer_t content_buffer = text_buffer_create(ctx->content_size);

    if (ctx->tn_size <= 0) {
        return;
    }

    int ret = arc_open(&arc_ctx, f, &a, &arc_data, TRUE);
    if (ret != ARCHIVE_OK) {
        CTX_LOG_ERRORF(f->filepath, "(ebook.c) [%d] %s", ret, archive_error_string(a))
        archive_read_free(a);
        return;
    }

    while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
        struct stat info = *archive_entry_stat(entry);
        if (S_ISREG(info.st_mode)) {
            const char *utf8_name = archive_entry_pathname_utf8(entry);
            const char *file_path = utf8_name == NULL ? archive_entry_pathname(entry) : utf8_name;

            char *p = strrchr(file_path, '.');
            if (p != NULL && (strcmp(p, ".html") == 0 || (strcmp(p, ".xhtml") == 0))) {
                size_t entry_size = archive_entry_size(entry);
                void *buf = malloc(entry_size + 1);
                size_t read = archive_read_data(a, buf, entry_size);
                *(char *) (buf + entry_size) = '\0';

                if (read != entry_size) {
                    const char *err_str = archive_error_string(a);
                    if (err_str) {
                        CTX_LOG_ERRORF("ebook.c", "Error while reading entry: %s", err_str)
                    }
                    free(buf);
                    break;
                }

                ret = text_buffer_append_markup(&content_buffer, buf);
                free(buf);

                if (ret == TEXT_BUF_FULL) {
                    break;
                }
            }
        }
    }

    text_buffer_terminate_string(&content_buffer);

    meta_line_t *meta_content = malloc(sizeof(meta_line_t) + content_buffer.dyn_buffer.cur);
    meta_content->key = MetaContent;
    memcpy(meta_content->str_val, content_buffer.dyn_buffer.buf, content_buffer.dyn_buffer.cur);
    APPEND_META(doc, meta_content)

    text_buffer_destroy(&content_buffer);

    archive_read_free(a);
}

void parse_ebook(scan_ebook_ctx_t *ctx, vfile_t *f, const char *mime_str, document_t *doc) {

    if (ctx->fast_epub_parse && is_epub(mime_str)) {
        parse_epub_fast(ctx, f, doc);
        return;
    }

    size_t buf_len;
    void *buf = read_all(f, &buf_len);
    if (buf == NULL) {
        CTX_LOG_ERROR(f->filepath, "read_all() failed")
        return;
    }

    parse_ebook_mem(ctx, buf, buf_len, mime_str, doc, FALSE);
    free(buf);
}
