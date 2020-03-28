#ifndef SCAN_TEXT_H
#define SCAN_TEXT_H

#include "../scan.h"
#include "../util.h"

typedef struct {
    long content_size;
} scan_text_ctx_t;

scan_code_t parse_text(scan_text_ctx_t *ctx, struct vfile *f, document_t *doc);

#endif
