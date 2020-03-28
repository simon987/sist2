#ifndef SCAN_FONT_H
#define SCAN_FONT_H

#include "../scan.h"


typedef struct {
    int enable_tn;
} scan_font_cxt_t;

void parse_font(scan_font_cxt_t *ctx, vfile_t *f, document_t *doc);
void cleanup_font();

#endif
