#ifndef SIST2_CTX_H
#define SIST2_CTX_H

#include "sist.h"

struct {
    struct index_t index;

    GHashTable *mime_table;
    GHashTable *ext_table;

    tpool_t *pool;

    int tn_size;
    int threads;
    int content_size;
    float tn_qscale;
    int depth;

    size_t stat_tn_size;
    size_t stat_index_size;

    GHashTable *original_table;
    GHashTable *copy_table;

    pthread_mutex_t mupdf_mu;
} ScanCtx;


#ifndef SIST_SCAN_ONLY
struct {
    char *es_url;
} IndexCtx;

struct {
    char *es_url;
    int index_count;
    char *b64credentials;
    struct index_t indices[16];
} WebCtx;
#endif


#endif
