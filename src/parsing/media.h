#ifndef SIST2_MEDIA_H
#define SIST2_MEDIA_H


#include "src/sist.h"

#define MIN_VIDEO_SIZE 1024 * 64
#define MIN_IMAGE_SIZE 1024 * 2

void parse_media_filename(const char * filepath, document_t *doc);

void parse_media_vfile(struct vfile *f, document_t *doc);

#endif
