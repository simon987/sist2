#ifndef SIST2_UTIL_H
#define SIST2_UTIL_H

#include <stdio.h>

#define TEXT_BUF_FULL -1
#define INITIAL_BUF_SIZE 1024 * 16

#define SHOULD_IGNORE_CHAR(c) !(SHOULD_KEEP_CHAR(c))
#define SHOULD_KEEP_CHAR(c) ((c >= '\'' && c <= ';') || (c >= 'A' && c <= 'z') || (c > 127))


typedef struct dyn_buffer {
    char *buf;
    size_t cur;
    size_t size;
} dyn_buffer_t;

#include "sist.h"

typedef struct text_buffer {
    size_t max_size;
    int last_char_was_whitespace;
    dyn_buffer_t dyn_buffer;
} text_buffer_t;

char *abspath(const char *path);

char *expandpath(const char *path);

dyn_buffer_t url_escape(char *str);

void progress_bar_print(double percentage, size_t tn_size, size_t index_size);

GHashTable *incremental_get_table();

__always_inline
static int utf8_validchr2(const char *s) {
    if (0x00 == (0x80 & *s)) {
        return TRUE;
    } else if (0xf0 == (0xf8 & *s)) {
        if ((0x80 != (0xc0 & s[1])) || (0x80 != (0xc0 & s[2])) ||
            (0x80 != (0xc0 & s[3]))) {
            return FALSE;
        }

        if (0x80 == (0xc0 & s[4])) {
            return FALSE;
        }

        if ((0 == (0x07 & s[0])) && (0 == (0x30 & s[1]))) {
            return FALSE;
        }
    } else if (0xe0 == (0xf0 & *s)) {
        if ((0x80 != (0xc0 & s[1])) || (0x80 != (0xc0 & s[2]))) {
            return FALSE;
        }

        if (0x80 == (0xc0 & s[3])) {
            return FALSE;
        }

        if ((0 == (0x0f & s[0])) && (0 == (0x20 & s[1]))) {
            return FALSE;
        }
    } else if (0xc0 == (0xe0 & *s)) {
        if (0x80 != (0xc0 & s[1])) {
            return FALSE;
        }

        if (0x80 == (0xc0 & s[2])) {
            return FALSE;
        }

        if (0 == (0x1e & s[0])) {
            return FALSE;
        }
    } else {
        return FALSE;
    }

    return TRUE;
}


__always_inline
static dyn_buffer_t dyn_buffer_create() {
    dyn_buffer_t buf;

    buf.size = INITIAL_BUF_SIZE;
    buf.cur = 0;
    buf.buf = malloc(INITIAL_BUF_SIZE);

    return buf;
}

__always_inline
static void grow_buffer(dyn_buffer_t *buf, size_t size) {
    if (buf->cur + size > buf->size) {
        do {
            buf->size *= 2;
        } while (buf->cur + size > buf->size);

        buf->buf = realloc(buf->buf, buf->size);
    }
}

__always_inline
static void grow_buffer_small(dyn_buffer_t *buf) {
    if (buf->cur + sizeof(long) > buf->size) {
        buf->size *= 2;
        buf->buf = realloc(buf->buf, buf->size);
    }
}

__always_inline
static void dyn_buffer_write(dyn_buffer_t *buf, void *data, size_t size) {
    grow_buffer(buf, size);

    memcpy(buf->buf + buf->cur, data, size);
    buf->cur += size;
}

__always_inline
static void dyn_buffer_write_char(dyn_buffer_t *buf, char c) {
    grow_buffer_small(buf);

    *(buf->buf + buf->cur) = c;
    buf->cur += sizeof(c);
}

__always_inline
static void dyn_buffer_write_str(dyn_buffer_t *buf, char *str) {
    dyn_buffer_write(buf, str, strlen(str));
    dyn_buffer_write_char(buf, '\0');
}

__always_inline
static void dyn_buffer_append_string(dyn_buffer_t *buf, char *str) {
    dyn_buffer_write(buf, str, strlen(str));
}

__always_inline
static void dyn_buffer_write_int(dyn_buffer_t *buf, int d) {
    grow_buffer_small(buf);

    *(int *) (buf->buf + buf->cur) = d;
    buf->cur += sizeof(int);
}

__always_inline
static void dyn_buffer_write_short(dyn_buffer_t *buf, short s) {
    grow_buffer_small(buf);

    *(short *) (buf->buf + buf->cur) = s;
    buf->cur += sizeof(short);
}

__always_inline
static void dyn_buffer_write_long(dyn_buffer_t *buf, unsigned long l) {
    grow_buffer_small(buf);

    *(unsigned long *) (buf->buf + buf->cur) = l;
    buf->cur += sizeof(unsigned long);
}

__always_inline
static void dyn_buffer_destroy(dyn_buffer_t *buf) {
    free(buf->buf);
}

__always_inline
static void text_buffer_destroy(text_buffer_t *buf) {
    dyn_buffer_destroy(&buf->dyn_buffer);
}

__always_inline
static text_buffer_t text_buffer_create(int max_size) {
    text_buffer_t text_buf;

    text_buf.dyn_buffer = dyn_buffer_create();
    text_buf.max_size = max_size;
    text_buf.last_char_was_whitespace = FALSE;

    return text_buf;
}

__always_inline
static int text_buffer_append_char(text_buffer_t *buf, int c) {

    if (SHOULD_IGNORE_CHAR(c) || c == ' ') {
        if (!buf->last_char_was_whitespace && buf->dyn_buffer.cur != 0) {
            dyn_buffer_write_char(&buf->dyn_buffer, ' ');
            buf->last_char_was_whitespace = TRUE;

            if (buf->max_size > 0 && buf->dyn_buffer.cur >= buf->max_size) {
                return TEXT_BUF_FULL;
            }
        }
    } else {
        buf->last_char_was_whitespace = FALSE;
        grow_buffer_small(&buf->dyn_buffer);

        if (0 == ((utf8_int32_t) 0xffffff80 & c)) {
            *(buf->dyn_buffer.buf + buf->dyn_buffer.cur++) = (char) c;
        } else if (0 == ((utf8_int32_t) 0xfffff800 & c)) {
            *(buf->dyn_buffer.buf + buf->dyn_buffer.cur++) = 0xc0 | (char) (c >> 6);
            *(buf->dyn_buffer.buf + buf->dyn_buffer.cur++) = 0x80 | (char) (c & 0x3f);
        } else if (0 == ((utf8_int32_t) 0xffff0000 & c)) {
            *(buf->dyn_buffer.buf + buf->dyn_buffer.cur++) = 0xe0 | (char) (c >> 12);
            *(buf->dyn_buffer.buf + buf->dyn_buffer.cur++) = 0x80 | (char) ((c >> 6) & 0x3f);
            *(buf->dyn_buffer.buf + buf->dyn_buffer.cur++) = 0x80 | (char) (c & 0x3f);
        } else {
            *(buf->dyn_buffer.buf + buf->dyn_buffer.cur++) = 0xf0 | (char) (c >> 18);
            *(buf->dyn_buffer.buf + buf->dyn_buffer.cur++) = 0x80 | (char) ((c >> 12) & 0x3f);
            *(buf->dyn_buffer.buf + buf->dyn_buffer.cur++) = 0x80 | (char) ((c >> 6) & 0x3f);
            *(buf->dyn_buffer.buf + buf->dyn_buffer.cur++) = 0x80 | (char) (c & 0x3f);
        }

        if (buf->max_size > 0 && buf->dyn_buffer.cur >= buf->max_size) {
            return TEXT_BUF_FULL;
        }
    }

    return 0;
}


__always_inline
static void text_buffer_terminate_string(text_buffer_t *buf) {
    if (buf->dyn_buffer.cur > 0 && *(buf->dyn_buffer.buf + buf->dyn_buffer.cur - 1) == ' ') {
        *(buf->dyn_buffer.buf + buf->dyn_buffer.cur - 1) = '\0';
    } else {
        dyn_buffer_write_char(&buf->dyn_buffer, '\0');
    }
}

__always_inline
static int text_buffer_append_string(text_buffer_t *buf, char *str, size_t len) {

    utf8_int32_t c;
    if (str == NULL || len < 1 ||
        (0xf0 == (0xf8 & str[0]) && len < 4) ||
        (0xe0 == (0xf0 & str[0]) && len < 3) ||
        (0xc0 == (0xe0 & str[0]) && len == 1) ||
        *(str) == 0) {
        return 0;
    }

    for (void *v = utf8codepoint(str, &c); c != '\0' && ((char *) v - str + 4) < len; v = utf8codepoint(v, &c)) {
        if (utf8_validchr2(v)) {
            text_buffer_append_char(buf, c);
        }
    }
    return 0;
}

__always_inline
static int text_buffer_append_string0(text_buffer_t *buf, char *str) {
    return text_buffer_append_string(buf, str, strlen(str));
}

__always_inline
static void incremental_put(GHashTable *table, unsigned long inode_no, int mtime) {
    g_hash_table_insert(table, (gpointer) inode_no, GINT_TO_POINTER(mtime));
}

__always_inline
static int incremental_get(GHashTable *table, unsigned long inode_no) {
    if (table != NULL) {
        return GPOINTER_TO_INT(g_hash_table_lookup(table, (gpointer) inode_no));
    } else {
        return 0;
    }
}

__always_inline
static int incremental_mark_file_for_copy(GHashTable *table, unsigned long inode_no) {
    g_hash_table_insert(table, GINT_TO_POINTER(inode_no), GINT_TO_POINTER(1));
}


const char *find_file_in_paths(const char **paths, const char *filename);

#endif
