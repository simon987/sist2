#ifndef SIST2_MEDIA_H
#define SIST2_MEDIA_H


#include "src/sist.h"

#define MIN_VIDEO_SIZE 1024 * 64

void parse_media(const char * filepath, document_t *doc);

#endif
