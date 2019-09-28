#ifndef SIST2_TYPES_H
#define SIST2_TYPES_H


#define META_INT_MASK 0xF0
#define META_STR_MASK 0xE0
#define META_LONG_MASK 0xD0
#define IS_META_INT(key) (key & META_INT_MASK) == META_INT_MASK
#define IS_META_LONG(key) (key & META_LONG_MASK) == META_LONG_MASK
#define IS_META_STR(meta) (meta->key & META_STR_MASK) == META_STR_MASK

// This is written to file as a 8bit char!
enum metakey {
    MetaContent = 1 | META_STR_MASK,
    MetaWidth = 2 | META_INT_MASK,
    MetaHeight = 3 | META_INT_MASK,
    MetaMediaDuration = 4 | META_LONG_MASK,
    MetaMediaAudioCodec = 5 | META_INT_MASK,
    MetaMediaVideoCodec = 6 | META_INT_MASK,
    MetaMediaBitrate = 7 | META_LONG_MASK,
    MetaArtist = 8 | META_STR_MASK,
    MetaAlbum = 9 | META_STR_MASK,
    MetaAlbumArtist = 10 | META_STR_MASK,
    MetaGenre = 11 | META_STR_MASK,
    MetaTitle = 12 | META_STR_MASK,
    MetaFontName = 13 | META_STR_MASK,
};

typedef struct index_descriptor {
    char uuid[UUID_STR_LEN];
    char version[6];
    long timestamp;
    char root[PATH_MAX];
    char rewrite_url[8196];
    short root_len;
    char name[1024];
} index_descriptor_t;

typedef struct index_t {
    struct index_descriptor desc;
    struct store_t *store;
    char path[PATH_MAX];
} index_t;

typedef struct meta_line {
    struct meta_line *next;
    enum metakey key;
    union {
        unsigned long longval;
        int intval;
        char strval[0];
    };
} meta_line_t;


typedef struct document {
    unsigned char uuid[16];
    unsigned long ino;
    unsigned long size;
    unsigned int mime;
    int mtime;
    short base;
    short ext;
    meta_line_t *meta_head;
    meta_line_t *meta_tail;
    char *filepath;
} document_t;

typedef struct parse_job_t {
    int base;
    int ext;
    struct stat info;
    char filepath[1];
} parse_job_t;


#define APPEND_META(doc, meta) \
    meta->next = NULL;\
    if (doc->meta_head == NULL) {\
        doc->meta_head = meta;\
        doc->meta_tail = doc->meta_head;\
    } else {\
        doc->meta_tail->next = meta;\
        doc->meta_tail = meta;\
    }

#endif
