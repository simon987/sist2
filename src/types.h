#ifndef SIST2_TYPES_H
#define SIST2_TYPES_H

#define INDEX_TYPE_NDJSON "ndjson"

typedef struct index_descriptor {
    char id[SIST_INDEX_ID_LEN];
    char version[64];
    long timestamp;
    char root[PATH_MAX];
    char rewrite_url[8192];
    short root_len;
    char name[1024];
    char type[64];
} index_descriptor_t;

typedef struct index_t {
    struct index_descriptor desc;
    struct store_t *store;
    struct store_t *tag_store;
    struct store_t *meta_store;
    char path[PATH_MAX];
} index_t;

#endif
