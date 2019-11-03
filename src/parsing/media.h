#ifndef SIST2_MEDIA_H
#define SIST2_MEDIA_H


#include "src/sist.h"

#define MIN_VIDEO_SIZE 1024 * 64
#define MIN_IMAGE_SIZE 1024 * 2

void parse_media(const char * filepath, document_t *doc);

#endif
