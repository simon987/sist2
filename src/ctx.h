#ifndef SIST2_CTX_H
#define SIST2_CTX_H

#include "sist.h"
#include "tpool.h"
#include "libscan/scan.h"
#include "libscan/arc/arc.h"
#include "libscan/comic/comic.h"
#include "libscan/ebook/ebook.h"
#include "libscan/font/font.h"
#include "libscan/media/media.h"
#include "libscan/ooxml/ooxml.h"
#include "libscan/text/text.h"
#include "libscan/mobi/scan_mobi.h"
#include "libscan/raw/raw.h"
#include "src/io/store.h"

#include <glib.h>
#include <pcre.h>

typedef struct {
    struct index_t index;

    GHashTable *mime_table;
    GHashTable *ext_table;

    tpool_t *pool;

    int threads;
    int depth;

    size_t stat_tn_size;
    size_t stat_index_size;

    GHashTable *original_table;
    GHashTable *copy_table;

    pcre *exclude;
    pcre_extra *exclude_extra;
    int fast;

    scan_arc_ctx_t arc_ctx;
    scan_comic_ctx_t comic_ctx;
    scan_ebook_ctx_t ebook_ctx;
    scan_font_ctx_t font_ctx;
    scan_media_ctx_t media_ctx;
    scan_ooxml_ctx_t ooxml_ctx;
    scan_text_ctx_t text_ctx;
    scan_mobi_ctx_t mobi_ctx;
    scan_raw_ctx_t raw_ctx;
} ScanCtx_t;

typedef struct {
    int verbose;
    int very_verbose;
    int no_color;
} LogCtx_t;

typedef struct {
    char *es_url;
    char *es_index;
    int batch_size;
    tpool_t *pool;
    store_t *tag_store;
    GHashTable *tags;
    store_t *meta_store;
    GHashTable *meta;
} IndexCtx_t;

typedef struct {
    char *es_url;
    char *es_index;
    int index_count;
    char *auth_user;
    char *auth_pass;
    int auth_enabled;
    int tag_auth_enabled;
    struct index_t indices[64];
} WebCtx_t;

extern ScanCtx_t ScanCtx;
extern WebCtx_t WebCtx;
extern IndexCtx_t IndexCtx;
extern LogCtx_t LogCtx;


#endif
