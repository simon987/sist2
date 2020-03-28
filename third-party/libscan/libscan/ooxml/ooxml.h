#ifndef SCAN_OOXML_H
#define SCAN_OOXML_H

#include <stdlib.h>
#include "../scan.h"

typedef struct {
    long content_size;
} scan_ooxml_cxt_t;

void parse_doc(scan_ooxml_cxt_t *ctx, vfile_t *f, document_t *doc);

#endif
