#ifndef SCAN_EBOOK_H
#define SCAN_EBOOK_H

#include "../scan.h"

typedef struct {
    long content_size;
    int tn_size;
    const char *tesseract_lang;
    const char *tesseract_path;
    pthread_mutex_t mupdf_mutex;
} scan_ebook_ctx_t;

void parse_ebook(scan_ebook_ctx_t *ctx, vfile_t *f, const char* mime_str,  document_t *doc);

#endif
