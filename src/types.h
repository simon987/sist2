#ifndef SIST2_TYPES_H
#define SIST2_TYPES_H

typedef struct database database_t;

typedef struct index_descriptor {
    char id[SIST_INDEX_ID_LEN];
    char version[64];
    int version_major;
    int version_minor;
    int version_patch;
    long timestamp;
    char root[PATH_MAX];
    char rewrite_url[8192];
    int root_len;
    char name[1024];
} index_descriptor_t;

typedef struct index_t {
    struct index_descriptor desc;

    database_t *db;

    char path[PATH_MAX];
} index_t;

#endif
