#ifndef SIST2_TYPES_H
#define SIST2_TYPES_H


#define META_INT_MASK 0x80
#define META_STR_MASK 0x40
#define META_LONG_MASK 0x20
#define IS_META_INT(key) (key & META_INT_MASK) == META_INT_MASK
#define IS_META_LONG(key) (key & META_LONG_MASK) == META_LONG_MASK
#define IS_META_STR(meta) (meta->key & META_STR_MASK) == META_STR_MASK

#define ARC_MODE_SKIP 0
#define ARC_MODE_LIST 1
#define ARC_MODE_SHALLOW 2
#define ARC_MODE_RECURSE 3
typedef int archive_mode_t;

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
    MetaParent = 14 | META_STR_MASK,
    MetaExifMake = 15 | META_STR_MASK,
    MetaExifSoftware = 16 | META_STR_MASK,
    MetaExifExposureTime = 17 | META_STR_MASK,
    MetaExifFNumber = 18 | META_STR_MASK,
    MetaExifFocalLength = 19 | META_STR_MASK,
    MetaExifUserComment = 20 | META_STR_MASK,
    MetaExifModel = 21 | META_STR_MASK,
    MetaExifIsoSpeedRatings = 22 | META_STR_MASK,
    //Note to self: this will break after 31 entries
};

#define INDEX_TYPE_BIN "binary"
#define INDEX_TYPE_JSON "json"
#define INDEX_VERSION_EXTERNAL "_external_v1"

typedef struct index_descriptor {
    char uuid[UUID_STR_LEN];
    char version[6];
    long timestamp;
    char root[PATH_MAX];
    char rewrite_url[8196];
    short root_len;
    char name[1024];
    char type[64];
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

typedef struct vfile vfile_t;

typedef int (*read_func_t)(struct vfile *, void *buf, size_t size);

typedef void (*close_func_t)(struct vfile *);

typedef struct vfile {

    union {
        int fd;
        struct archive *arc;
    };

    int is_fs_file;
    char *filepath;

    read_func_t read;
    close_func_t close;
} vfile_t;

typedef struct parse_job_t {
    int base;
    int ext;
    struct stat info;
    struct vfile vfile;
    uuid_t parent;
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
