#ifndef SCAN_SCAN_H
#define SCAN_SCAN_H

#include <stdio.h>
#include <sys/stat.h>
#include <uuid/uuid.h>

#include "macros.h"


#define META_INT_MASK 0x80
#define META_STR_MASK 0x40
#define META_LONG_MASK 0x20

#define UNUSED(x) __attribute__((__unused__))  x

#define META_STR(id) ((unsigned) id) | ((unsigned) META_STR_MASK)
#define META_INT(id) ((unsigned) id) | ((unsigned) META_INT_MASK)
#define META_LONG(id) ((unsigned) id) | ((unsigned) META_LONG_MASK)

#define IS_META_INT(key) (key & META_INT_MASK) == META_INT_MASK
#define IS_META_LONG(key) (key & META_LONG_MASK) == META_LONG_MASK
#define IS_META_STR(meta) (meta->key & META_STR_MASK) == META_STR_MASK


typedef int scan_code_t;
#define SCAN_OK (scan_code_t) 0
#define SCAN_ERR_READ (scan_code_t) -1

// This is written to file as a 16-bit int!
enum metakey {
    MetaContent = META_STR(1),
    MetaWidth = META_INT(2),
    MetaHeight = META_INT(3),
    MetaMediaDuration = META_LONG(4),
    MetaMediaAudioCodec = META_INT(5),
    MetaMediaVideoCodec = META_INT(6),
    MetaMediaBitrate = META_LONG(7),
    MetaArtist = META_STR(8),
    MetaAlbum = META_STR(9),
    MetaAlbumArtist = META_STR(10),
    MetaGenre = META_STR(11),
    MetaTitle = META_STR(12),
    MetaFontName = META_STR(13),
    MetaParent = META_STR(14),
    MetaExifMake = META_STR(15),
    MetaExifSoftware = META_STR(16),
    MetaExifExposureTime = META_STR(17),
    MetaExifFNumber = META_STR(18),
    MetaExifFocalLength = META_STR(19),
    MetaExifUserComment = META_STR(20),
    MetaExifModel = META_STR(21),
    MetaExifIsoSpeedRatings = META_STR(22),
    MetaExifDateTime = META_STR(23),
};

typedef struct meta_line {
    struct meta_line *next;
    enum metakey key;
    union {
        char str_val[0];
        int int_val;
        unsigned long long_val;
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

__attribute__((warn_unused_result))
typedef int (*read_func_t)(struct vfile *, void *buf, size_t size);

typedef void (*close_func_t)(struct vfile *);

typedef struct vfile {
    union {
        int fd;
        struct archive *arc;
    };

    int is_fs_file;
    char *filepath;
    struct stat info;

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

#include "arc/arc.h"
#include "cbr/cbr.h"
#include "ebook/ebook.h"
#include "font/font.h"
#include "media/media.h"
#include "ooxml/ooxml.h"
#include "text/text.h"
