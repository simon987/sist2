#include "media.h"

#include "../util.h"

#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libswresample/swresample.h"
#include "libavcodec/avcodec.h"
#include "libavutil/imgutils.h"

#include <ctype.h>

#define MIN_SIZE 32
#define AVIO_BUF_SIZE 8192

__always_inline
static AVCodecContext *alloc_jpeg_encoder(int dstW, int dstH, float qscale) {

    AVCodec *jpeg_codec = avcodec_find_encoder(AV_CODEC_ID_MJPEG);
    AVCodecContext *jpeg = avcodec_alloc_context3(jpeg_codec);
    jpeg->width = dstW;
    jpeg->height = dstH;
    jpeg->time_base.den = 1000000;
    jpeg->time_base.num = 1;
    jpeg->i_quant_factor = qscale;

    jpeg->pix_fmt = AV_PIX_FMT_YUVJ420P;
    int ret = avcodec_open2(jpeg, jpeg_codec, NULL);

    if (ret != 0) {
        printf("Could not open jpeg encoder: %s!\n", av_err2str(ret));
        return NULL;
    }

    return jpeg;
}

__always_inline
AVFrame *scale_frame(const AVCodecContext *decoder, const AVFrame *frame, int size) {

    int dstW;
    int dstH;
    if (frame->width <= size && frame->height <= size) {
        dstW = frame->width;
        dstH = frame->height;
    } else {
        double ratio = (double) frame->width / frame->height;
        if (frame->width > frame->height) {
            dstW = size;
            dstH = (int) (size / ratio);
        } else {
            dstW = (int) (size * ratio);
            dstH = size;
        }
    }

    if (dstW <= MIN_SIZE || dstH <= MIN_SIZE) {
        return NULL;
    }

    AVFrame *scaled_frame = av_frame_alloc();

    struct SwsContext *ctx = sws_getContext(
            decoder->width, decoder->height, decoder->pix_fmt,
            dstW, dstH, AV_PIX_FMT_YUVJ420P,
            SWS_FAST_BILINEAR, 0, 0, 0
    );

    int dst_buf_len = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, dstW, dstH, 1);
    uint8_t *dst_buf = (uint8_t *) av_malloc(dst_buf_len);

    av_image_fill_arrays(scaled_frame->data, scaled_frame->linesize, dst_buf, AV_PIX_FMT_YUV420P, dstW, dstH, 1);

    sws_scale(ctx,
              (const uint8_t *const *) frame->data, frame->linesize,
              0, decoder->height,
              scaled_frame->data, scaled_frame->linesize
    );

    scaled_frame->width = dstW;
    scaled_frame->height = dstH;
    scaled_frame->format = AV_PIX_FMT_YUV420P;

    sws_freeContext(ctx);

    return scaled_frame;
}

__always_inline
static AVFrame *read_frame(AVFormatContext *pFormatCtx, AVCodecContext *decoder, int stream_idx, document_t *doc) {
    AVFrame *frame = av_frame_alloc();

    AVPacket avPacket;
    av_init_packet(&avPacket);

    int receive_ret = -EAGAIN;
    while (receive_ret == -EAGAIN) {
        // Get video frame
        while (1) {
            int read_frame_ret = av_read_frame(pFormatCtx, &avPacket);

            if (read_frame_ret != 0) {
                if (read_frame_ret != AVERROR_EOF) {
//                    LOG_WARNINGF(doc->filepath,
//                                 "(media.c) avcodec_read_frame() returned error code [%d] %s",
//                                 read_frame_ret, av_err2str(read_frame_ret)
//                    )
                }
                av_frame_free(&frame);
                av_packet_unref(&avPacket);
                return NULL;
            }

            //Ignore audio/other frames
            if (avPacket.stream_index != stream_idx) {
                av_packet_unref(&avPacket);
                continue;
            }
            break;
        }

        // Feed it to decoder
        int decode_ret = avcodec_send_packet(decoder, &avPacket);
        if (decode_ret != 0) {
//            LOG_ERRORF(doc->filepath,
//                         "(media.c) avcodec_send_packet() returned error code [%d] %s",
//                         decode_ret, av_err2str(decode_ret)
//            )
            av_frame_free(&frame);
            av_packet_unref(&avPacket);
            return NULL;
        }
        av_packet_unref(&avPacket);
        receive_ret = avcodec_receive_frame(decoder, frame);
    }
    return frame;
}

#define APPEND_TAG_META(doc, tag_, keyname) \
    text_buffer_t tex = text_buffer_create(-1); \
    text_buffer_append_string0(&tex, tag_->value); \
    text_buffer_terminate_string(&tex); \
    meta_line_t *meta_tag = malloc(sizeof(meta_line_t) + tex.dyn_buffer.cur); \
    meta_tag->key = keyname; \
    strcpy(meta_tag->str_val, tex.dyn_buffer.buf); \
    APPEND_META(doc, meta_tag) \
    text_buffer_destroy(&tex);

__always_inline
static void append_audio_meta(AVFormatContext *pFormatCtx, document_t *doc) {

    AVDictionaryEntry *tag = NULL;
    while ((tag = av_dict_get(pFormatCtx->metadata, "", tag, AV_DICT_IGNORE_SUFFIX))) {
        char key[256];
        strncpy(key, tag->key, sizeof(key));

        char *ptr = key;
        for (; *ptr; ++ptr) *ptr = (char) tolower(*ptr);

        if (strcmp(key, "artist") == 0) {
            APPEND_TAG_META(doc, tag, MetaArtist)
        } else if (strcmp(key, "genre") == 0) {
            APPEND_TAG_META(doc, tag, MetaGenre)
        } else if (strcmp(key, "title") == 0) {
            APPEND_TAG_META(doc, tag, MetaTitle)
        } else if (strcmp(key, "album_artist") == 0) {
            APPEND_TAG_META(doc, tag, MetaAlbumArtist)
        } else if (strcmp(key, "album") == 0) {
            APPEND_TAG_META(doc, tag, MetaAlbum)
        }
    }
}

__always_inline
static void
append_video_meta(AVFormatContext *pFormatCtx, AVFrame *frame, document_t *doc, int include_audio_tags, int is_video) {

    if (is_video) {
        meta_line_t *meta_duration = malloc(sizeof(meta_line_t));
        meta_duration->key = MetaMediaDuration;
        meta_duration->long_val = pFormatCtx->duration / AV_TIME_BASE;
        APPEND_META(doc, meta_duration)

        meta_line_t *meta_bitrate = malloc(sizeof(meta_line_t));
        meta_bitrate->key = MetaMediaBitrate;
        meta_bitrate->long_val = pFormatCtx->bit_rate;
        APPEND_META(doc, meta_bitrate)
    }

    AVDictionaryEntry *tag = NULL;
    if (is_video) {
        while ((tag = av_dict_get(pFormatCtx->metadata, "", tag, AV_DICT_IGNORE_SUFFIX))) {
            if (include_audio_tags && strcmp(tag->key, "title") == 0) {
                APPEND_TAG_META(doc, tag, MetaTitle)
            } else if (strcmp(tag->key, "comment") == 0) {
                APPEND_TAG_META(doc, tag, MetaContent)
            } else if (include_audio_tags && strcmp(tag->key, "artist") == 0) {
                APPEND_TAG_META(doc, tag, MetaArtist)
            }
        }
    } else {
        // EXIF metadata
        while ((tag = av_dict_get(frame->metadata, "", tag, AV_DICT_IGNORE_SUFFIX))) {
            if (include_audio_tags && strcmp(tag->key, "Artist") == 0) {
                APPEND_TAG_META(doc, tag, MetaArtist)
            } else if (strcmp(tag->key, "ImageDescription") == 0) {
                APPEND_TAG_META(doc, tag, MetaContent)
            } else if (strcmp(tag->key, "Make") == 0) {
                APPEND_TAG_META(doc, tag, MetaExifMake)
            } else if (strcmp(tag->key, "Model") == 0) {
                APPEND_TAG_META(doc, tag, MetaExifModel)
            } else if (strcmp(tag->key, "Software") == 0) {
                APPEND_TAG_META(doc, tag, MetaExifSoftware)
            } else if (strcmp(tag->key, "FNumber") == 0) {
                APPEND_TAG_META(doc, tag, MetaExifFNumber)
            } else if (strcmp(tag->key, "FocalLength") == 0) {
                APPEND_TAG_META(doc, tag, MetaExifFocalLength)
            } else if (strcmp(tag->key, "UserComment") == 0) {
                APPEND_TAG_META(doc, tag, MetaExifUserComment)
            } else if (strcmp(tag->key, "ISOSpeedRatings") == 0) {
                APPEND_TAG_META(doc, tag, MetaExifIsoSpeedRatings)
            } else if (strcmp(tag->key, "ExposureTime") == 0) {
                APPEND_TAG_META(doc, tag, MetaExifExposureTime)
            } else if (strcmp(tag->key, "DateTime") == 0) {
                APPEND_TAG_META(doc, tag, MetaExifDateTime)
            }
        }
    }
}

void parse_media_format_ctx(scan_media_ctx_t *ctx, AVFormatContext *pFormatCtx, document_t *doc) {

    int video_stream = -1;
    int audio_stream = -1;

    avformat_find_stream_info(pFormatCtx, NULL);

    for (int i = (int) pFormatCtx->nb_streams - 1; i >= 0; i--) {
        AVStream *stream = pFormatCtx->streams[i];

        if (stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            if (audio_stream == -1) {
                meta_line_t *meta_audio = malloc(sizeof(meta_line_t));
                meta_audio->key = MetaMediaAudioCodec;
                meta_audio->int_val = stream->codecpar->codec_id;
                APPEND_META(doc, meta_audio)

                append_audio_meta(pFormatCtx, doc);
                audio_stream = i;
            }
        } else if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {

            if (video_stream == -1) {
                meta_line_t *meta_vid = malloc(sizeof(meta_line_t));
                meta_vid->key = MetaMediaVideoCodec;
                meta_vid->int_val = stream->codecpar->codec_id;
                APPEND_META(doc, meta_vid)

                meta_line_t *meta_w = malloc(sizeof(meta_line_t));
                meta_w->key = MetaWidth;
                meta_w->int_val = stream->codecpar->width;
                APPEND_META(doc, meta_w)

                meta_line_t *meta_h = malloc(sizeof(meta_line_t));
                meta_h->key = MetaHeight;
                meta_h->int_val = stream->codecpar->height;
                APPEND_META(doc, meta_h)

                video_stream = i;
            }
        }
    }

    if (video_stream != -1 && ctx->tn_size > 0) {
        AVStream *stream = pFormatCtx->streams[video_stream];

        if (stream->codecpar->width <= MIN_SIZE || stream->codecpar->height <= MIN_SIZE) {
            avformat_close_input(&pFormatCtx);
            avformat_free_context(pFormatCtx);
            return;
        }

        // Decoder
        AVCodec *video_codec = avcodec_find_decoder(stream->codecpar->codec_id);
        AVCodecContext *decoder = avcodec_alloc_context3(video_codec);
        avcodec_parameters_to_context(decoder, stream->codecpar);
        avcodec_open2(decoder, video_codec, NULL);

        //Seek
        if (stream->nb_frames > 1 && stream->codecpar->codec_id != AV_CODEC_ID_GIF) {
            int seek_ret = 0;
            for (int i = 20; i >= 0; i--) {
                seek_ret = av_seek_frame(pFormatCtx, video_stream,
                                         stream->duration * 0.10, 0);
                if (seek_ret == 0) {
                    break;
                }
            }
        }

        AVFrame *frame = read_frame(pFormatCtx, decoder, video_stream, doc);
        if (frame == NULL) {
            avcodec_free_context(&decoder);
            avformat_close_input(&pFormatCtx);
            avformat_free_context(pFormatCtx);
            return;
        }

        append_video_meta(pFormatCtx, frame, doc, audio_stream == -1, stream->nb_frames > 1);

        // Scale frame
        AVFrame *scaled_frame = scale_frame(decoder, frame, ctx->tn_size);

        if (scaled_frame == NULL) {
            av_frame_free(&frame);
            avcodec_free_context(&decoder);
            avformat_close_input(&pFormatCtx);
            avformat_free_context(pFormatCtx);
            return;
        }

        // Encode frame to jpeg
        AVCodecContext *jpeg_encoder = alloc_jpeg_encoder(scaled_frame->width, scaled_frame->height, ctx->tn_qscale);
        avcodec_send_frame(jpeg_encoder, scaled_frame);

        AVPacket jpeg_packet;
        av_init_packet(&jpeg_packet);
        avcodec_receive_packet(jpeg_encoder, &jpeg_packet);

        // Save thumbnail
//        store_write(ScanCtx.index.store, (char *) doc->uuid, sizeof(doc->uuid), (char *) jpeg_packet.data,
//                    jpeg_packet.size);

        av_packet_unref(&jpeg_packet);
        av_frame_free(&frame);
        av_free(*scaled_frame->data);
        av_frame_free(&scaled_frame);
        avcodec_free_context(&jpeg_encoder);
        avcodec_free_context(&decoder);
    }

    avformat_close_input(&pFormatCtx);
    avformat_free_context(pFormatCtx);
}

void parse_media_filename(scan_media_ctx_t *ctx, const char *filepath, document_t *doc) {

    AVFormatContext *pFormatCtx = avformat_alloc_context();
    if (pFormatCtx == NULL) {
//        LOG_ERROR(doc->filepath, "(media.c) Could not allocate context with avformat_alloc_context()")
        return;
    }
    int res = avformat_open_input(&pFormatCtx, filepath, NULL, NULL);
    if (res < 0) {
//        LOG_ERRORF(doc->filepath, "(media.c) avformat_open_input() returned [%d] %s", res, av_err2str(res))
        avformat_close_input(&pFormatCtx);
        avformat_free_context(pFormatCtx);
        return;
    }

    parse_media_format_ctx(ctx, pFormatCtx, doc);
}


int vfile_read(void *ptr, uint8_t *buf, int buf_size) {
    struct vfile *f = ptr;

    int ret = f->read(f, buf, buf_size);

    if (ret == 0) {
        return AVERROR_EOF;
    }
    return ret;
}

void parse_media_vfile(scan_media_ctx_t *ctx, struct vfile *f, document_t *doc) {

    AVFormatContext *pFormatCtx = avformat_alloc_context();
    if (pFormatCtx == NULL) {
//        LOG_ERROR(doc->filepath, "(media.c) Could not allocate context with avformat_alloc_context()")
        return;
    }

    unsigned char *buffer = (unsigned char *) av_malloc(AVIO_BUF_SIZE);
    AVIOContext *io_ctx = avio_alloc_context(buffer, AVIO_BUF_SIZE, 0, f, vfile_read, NULL, NULL);

    pFormatCtx->pb = io_ctx;
    pFormatCtx->flags |= AVFMT_FLAG_CUSTOM_IO;

    int res = avformat_open_input(&pFormatCtx, "", NULL, NULL);
    if (res == -5) {
        // Tried to parse media that requires seek
        av_free(io_ctx->buffer);
        avio_context_free(&io_ctx);
        avformat_close_input(&pFormatCtx);
        avformat_free_context(pFormatCtx);
        return;
    } else if (res < 0) {
//        LOG_ERRORF(doc->filepath, "(media.c) avformat_open_input() returned [%d] %s", res, av_err2str(res))
        av_free(io_ctx->buffer);
        avio_context_free(&io_ctx);
        avformat_close_input(&pFormatCtx);
        avformat_free_context(pFormatCtx);
        return;
    }

    parse_media_format_ctx(ctx, pFormatCtx, doc);
    av_free(io_ctx->buffer);
    avio_context_free(&io_ctx);
}

void parse_media(scan_media_ctx_t *ctx, vfile_t *f, document_t *doc) {

    if (f->is_fs_file) {
        parse_media_filename(ctx, f->filepath, doc);
    } else {
        parse_media_vfile(ctx, f, doc);
    }
}
