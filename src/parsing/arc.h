#ifndef SIST2_ARC_H
#define SIST2_ARC_H

#include "src/sist.h"

int should_parse_filtered_file(const char *filepath, int ext);

void parse_archive(vfile_t *f, document_t *doc);

int arc_read(struct vfile * f, void *buf, size_t size);

#endif
