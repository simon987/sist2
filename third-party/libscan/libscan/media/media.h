#ifndef SIST2_MEDIA_H
#define SIST2_MEDIA_H


#include "../scan.h"

#define MIN_VIDEO_SIZE 1024 * 64
#define MIN_IMAGE_SIZE 1024 * 2

typedef struct {
    long content_size;
    int tn_size;
    float tn_qscale;
} scan_media_ctx_t;

void parse_media(scan_media_ctx_t *ctx, vfile_t *f, document_t *doc);

#endif
