#ifndef SIST2_MIME_H
#define SIST2_MIME_H

#include "src/sist.h"

#define MAJOR_MIME(mime_id) (mime_id & 0x0FFF0000) >> 16

#define MIME_EMPTY 1

#define DONT_PARSE 0x80000000
#define SHOULD_PARSE(mime_id) (mime_id & DONT_PARSE) != DONT_PARSE

#define PDF_MASK 0x40000000
#define IS_PDF(mime_id) (mime_id & PDF_MASK) == PDF_MASK

#define FONT_MASK 0x20000000
#define IS_FONT(mime_id) (mime_id & FONT_MASK) == FONT_MASK

enum major_mime {
    MimeInvalid = 0,
    MimeModel = 1,
    MimeExample = 2,
    MimeMessage = 3,
    MimeMultipart = 4,
    MimeFont = 5,
    MimeVideo = 6,
    MimeAudio = 7,
    MimeImage = 8,
    MimeText = 9,
    MimeApplication = 10,
};

enum mime;

GHashTable *mime_get_mime_table();

GHashTable *mime_get_ext_table();

char *mime_get_mime_text(unsigned int);

unsigned int mime_get_mime_by_ext(GHashTable *ext_table, const char * ext);

unsigned int mime_get_mime_by_string(GHashTable *mime_table, const char * str);

#endif
