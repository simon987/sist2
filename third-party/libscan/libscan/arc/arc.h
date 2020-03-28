#ifndef SCAN_ARC_H
#define SCAN_ARC_H

#include <archive.h>
#include <archive_entry.h>
#include "../scan.h"

#define ARC_MODE_SKIP 0
#define ARC_MODE_LIST 1
#define ARC_MODE_SHALLOW 2
#define ARC_MODE_RECURSE 3
typedef int archive_mode_t;

typedef struct {
    archive_mode_t mode;
} scan_arc_ctx_t;

#define ARC_BUF_SIZE 8192

int should_parse_filtered_file(const char *filepath, int ext);

scan_code_t parse_archive(scan_arc_ctx_t *ctx, vfile_t *f, document_t *doc);

int arc_read(struct vfile * f, void *buf, size_t size);

#endif
