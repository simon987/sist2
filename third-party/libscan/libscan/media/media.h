#ifndef SIST2_MEDIA_H
#define SIST2_MEDIA_H


#include "../scan.h"

#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libswresample/swresample.h"
#include "libavcodec/avcodec.h"
#include "libavutil/imgutils.h"

typedef struct {
    log_callback_t log;
    logf_callback_t logf;
    store_callback_t store;

    int tn_size;
    float tn_qscale;
    long max_media_buffer;
    int read_subtitles;
} scan_media_ctx_t;

__always_inline
static AVCodecContext *alloc_jpeg_encoder(int w, int h, float qscale) {

    const AVCodec *jpeg_codec = avcodec_find_encoder(AV_CODEC_ID_MJPEG);
    AVCodecContext *jpeg = avcodec_alloc_context3(jpeg_codec);
    jpeg->width = w;
    jpeg->height = h;
    jpeg->time_base.den = 1000000;
    jpeg->time_base.num = 1;
    jpeg->i_quant_factor = qscale;

    jpeg->pix_fmt = AV_PIX_FMT_YUVJ420P;
    int ret = avcodec_open2(jpeg, jpeg_codec, NULL);

    if (ret != 0) {
        return NULL;
    }

    return jpeg;
}


void parse_media(scan_media_ctx_t *ctx, vfile_t *f, document_t *doc, const char*mime_str);

void init_media();

int store_image_thumbnail(scan_media_ctx_t *ctx, void *buf, size_t buf_len, document_t *doc, const char *url);

#endif
