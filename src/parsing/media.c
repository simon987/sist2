#include "src/sist.h"
#include "src/ctx.h"

AVCodecContext *alloc_jpeg_encoder(int dstW, int dstH, float qscale) {

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

AVFrame *scale_frame(const AVCodecContext *decoder, const AVFrame *frame, int size) {
    AVFrame *scaled_frame = av_frame_alloc();

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

    struct SwsContext *ctx = sws_getContext(
            decoder->width, decoder->height, decoder->pix_fmt,
            dstW, dstH, AV_PIX_FMT_YUVJ420P,
            SWS_FAST_BILINEAR, 0, 0, 0
    );

    int dst_buf_len = avpicture_get_size(AV_PIX_FMT_YUVJ420P, dstW, dstH);
    uint8_t *dst_buf = (uint8_t *) av_malloc(dst_buf_len);

    avpicture_fill((AVPicture *) scaled_frame, dst_buf, AV_PIX_FMT_YUVJ420P, dstW, dstH);

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

AVFrame *read_frame(AVFormatContext *pFormatCtx, AVCodecContext *decoder, int stream_idx) {
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
                    fprintf(stderr, "Error reading frame: %s\n", av_err2str(read_frame_ret));
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
            printf("Error decoding frame: %s\n", av_err2str(decode_ret));
        }
        av_packet_unref(&avPacket);
        receive_ret = avcodec_receive_frame(decoder, frame);
    }
    return frame;
}

#define APPEND_TAG_META(doc, tag, keyname) \
    text_buffer_t tex = text_buffer_create(4096); \
    text_buffer_append_string(&tex, tag->value); \
    meta_line_t *meta_tag = malloc(sizeof(meta_line_t) + tex.dyn_buffer.cur); \
    meta_tag->key = keyname; \
    strcpy(meta_tag->strval, tex.dyn_buffer.buf); \
    APPEND_META(doc, meta_tag) \
    text_buffer_destroy(&tex);

void append_audio_meta(AVFormatContext *pFormatCtx, document_t *doc) {

    AVDictionaryEntry *tag = NULL;
    while ((tag = av_dict_get(pFormatCtx->metadata, "", tag, AV_DICT_IGNORE_SUFFIX))) {
        char *key = tag->key;
        for (; *key; ++key) *key = (char) tolower(*key);

        if (strcmp(tag->key, "artist") == 0) {
            APPEND_TAG_META(doc, tag, MetaArtist)
        } else if (strcmp(tag->key, "genre") == 0) {
            APPEND_TAG_META(doc, tag, MetaGenre)
        } else if (strcmp(tag->key, "title") == 0) {
            APPEND_TAG_META(doc, tag, MetaTitle)
        } else if (strcmp(tag->key, "album_artist") == 0) {
            APPEND_TAG_META(doc, tag, MetaAlbumArtist)
        } else if (strcmp(tag->key, "album") == 0) {
            APPEND_TAG_META(doc, tag, MetaAlbum)
        }
    }
}

void append_video_meta(AVFormatContext *pFormatCtx, document_t *doc, int include_audio_tags) {

    meta_line_t *meta_duration = malloc(sizeof(meta_line_t));
    meta_duration->key = MetaMediaDuration;
    meta_duration->longval = pFormatCtx->duration / AV_TIME_BASE;
    APPEND_META(doc, meta_duration)

    meta_line_t *meta_bitrate = malloc(sizeof(meta_line_t));
    meta_bitrate->key = MetaMediaBitrate;
    meta_bitrate->intval = pFormatCtx->bit_rate;
    APPEND_META(doc, meta_bitrate)

    AVDictionaryEntry *tag = NULL;
    while ((tag = av_dict_get(pFormatCtx->metadata, "", tag, AV_DICT_IGNORE_SUFFIX))) {
        char *key = tag->key;
        for (; *key; ++key) *key = (char) tolower(*key);

        if (strcmp(tag->key, "title") == 0 && include_audio_tags) {
            APPEND_TAG_META(doc, tag, MetaTitle)
        } else if (strcmp(tag->key, "comment") == 0) {
            APPEND_TAG_META(doc, tag, MetaContent)
        }
    }
}

void parse_media(const char *filepath, document_t *doc) {

    int video_stream = -1;
    int audio_stream = -1;

    AVFormatContext *pFormatCtx = avformat_alloc_context();
    if (pFormatCtx == NULL) {
        fprintf(stderr, "Could not allocate AVFormatContext! %s \n", filepath);
        return;
    }
    int res = avformat_open_input(&pFormatCtx, filepath, NULL, NULL);
    if (res < 0) {
        printf("ERR%s %s\n", filepath, av_err2str(res));
        return;
    }

    avformat_find_stream_info(pFormatCtx, NULL);

    for (int i = (int) pFormatCtx->nb_streams - 1; i >= 0; i--) {
        AVStream *stream = pFormatCtx->streams[i];

        if (stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            if (audio_stream == -1) {
                meta_line_t *meta_audio = malloc(sizeof(meta_line_t));
                meta_audio->key = MetaMediaAudioCodec;
                meta_audio->intval = stream->codecpar->codec_id;
                APPEND_META(doc, meta_audio)

                append_audio_meta(pFormatCtx, doc);
                audio_stream = i;
            }
        } else if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {

            if (video_stream == -1) {
                meta_line_t *meta_vid = malloc(sizeof(meta_line_t));
                meta_vid->key = MetaMediaVideoCodec;
                meta_vid->intval = stream->codecpar->codec_id;
                APPEND_META(doc, meta_vid)

                meta_line_t *meta_w = malloc(sizeof(meta_line_t));
                meta_w->key = MetaWidth;
                meta_w->intval = stream->codecpar->width;
                APPEND_META(doc, meta_w)

                meta_line_t *meta_h = malloc(sizeof(meta_line_t));
                meta_h->key = MetaHeight;
                meta_h->intval = stream->codecpar->height;
                APPEND_META(doc, meta_h)

                video_stream = i;
            }
        }
    }

    if (video_stream != -1) {
        AVStream *stream = pFormatCtx->streams[video_stream];

        if (stream->nb_frames > 1) {
            //This is a video (not a still image)
            append_video_meta(pFormatCtx, doc, audio_stream == -1);
        }

        if (stream->codecpar->width <= 20 || stream->codecpar->height <= 20) {
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

        AVFrame *frame = read_frame(pFormatCtx, decoder, video_stream);
        if (frame == NULL) {
            avcodec_free_context(&decoder);
            avformat_close_input(&pFormatCtx);
            avformat_free_context(pFormatCtx);
            return;
        }

        // Scale frame
        AVFrame *scaled_frame = scale_frame(decoder, frame, ScanCtx.tn_size);

        // Encode frame to jpeg
        AVCodecContext *jpeg_encoder = alloc_jpeg_encoder(scaled_frame->width, scaled_frame->height, ScanCtx.tn_qscale);
        avcodec_send_frame(jpeg_encoder, scaled_frame);

        AVPacket jpeg_packet;
        av_init_packet(&jpeg_packet);
        avcodec_receive_packet(jpeg_encoder, &jpeg_packet);

        // Save thumbnail
        store_write(ScanCtx.index.store, (char *) doc->uuid, sizeof(doc->uuid), (char *) jpeg_packet.data, jpeg_packet.size);

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

