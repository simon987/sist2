#include "util.h"

dyn_buffer_t dyn_buffer_create() {
    dyn_buffer_t buf;

    buf.size = INITIAL_BUF_SIZE;
    buf.cur = 0;
    buf.buf = malloc(INITIAL_BUF_SIZE);

    return buf;
}

void grow_buffer(dyn_buffer_t *buf, size_t size) {
    if (buf->cur + size > buf->size) {
        do {
            buf->size *= 2;
        } while (buf->cur + size > buf->size);

        buf->buf = realloc(buf->buf, buf->size);
    }
}

void grow_buffer_small(dyn_buffer_t *buf) {
    if (buf->cur + sizeof(long) > buf->size) {
        buf->size *= 2;
        buf->buf = realloc(buf->buf, buf->size);
    }
}

void dyn_buffer_write(dyn_buffer_t *buf, void *data, size_t size) {
    grow_buffer(buf, size);

    memcpy(buf->buf + buf->cur, data, size);
    buf->cur += size;
}

void dyn_buffer_write_char(dyn_buffer_t *buf, char c) {
    grow_buffer_small(buf);

    *(buf->buf + buf->cur) = c;
    buf->cur += sizeof(c);
}

void dyn_buffer_write_str(dyn_buffer_t *buf, char *str) {
    dyn_buffer_write(buf, str, strlen(str));
    dyn_buffer_write_char(buf, '\0');
}

void dyn_buffer_write_int(dyn_buffer_t *buf, int d) {
    grow_buffer_small(buf);

    *(int *) (buf->buf + buf->cur) = d;
    buf->cur += sizeof(int);
}

void dyn_buffer_write_short(dyn_buffer_t *buf, short s) {
    grow_buffer_small(buf);

    *(short *) (buf->buf + buf->cur) = s;
    buf->cur += sizeof(short);
}

void dyn_buffer_write_long(dyn_buffer_t *buf, unsigned long l) {
    grow_buffer_small(buf);

    *(unsigned long *) (buf->buf + buf->cur) = l;
    buf->cur += sizeof(unsigned long);
}

void dyn_buffer_destroy(dyn_buffer_t *buf) {
    free(buf->buf);
}

void text_buffer_destroy(text_buffer_t *buf) {
    dyn_buffer_destroy(&buf->dyn_buffer);
}

text_buffer_t text_buffer_create(int max_size) {
    text_buffer_t text_buf;

    text_buf.dyn_buffer = dyn_buffer_create();
    text_buf.max_size = max_size;
    text_buf.last_char_was_whitespace = FALSE;

    return text_buf;
}

void text_buffer_terminate_string(text_buffer_t *buf) {
    dyn_buffer_write_char(&buf->dyn_buffer, '\0');
}

int text_buffer_append_char(text_buffer_t *buf, int c) {

    if (SHOULD_IGNORE_CHAR(c)) {
        if (!buf->last_char_was_whitespace) {
            dyn_buffer_write_char(&buf->dyn_buffer, ' ');
            buf->last_char_was_whitespace = TRUE;

            if (buf->dyn_buffer.cur >= buf->max_size) {
                return TEXT_BUF_FULL;
            }
        }
    } else {
        buf->last_char_was_whitespace = FALSE;
        dyn_buffer_write_char(&buf->dyn_buffer, (char) c);

        if (buf->dyn_buffer.cur >= buf->max_size) {
            return TEXT_BUF_FULL;
        }
    }

    return 0;
}

void incremental_put(GHashTable *table, unsigned long inode_no, int mtime) {
    g_hash_table_insert(table, (gpointer) inode_no, GINT_TO_POINTER(mtime));
}

int incremental_get(GHashTable *table, unsigned long inode_no) {
    if (table != NULL) {
        return GPOINTER_TO_INT(g_hash_table_lookup(table, (gpointer) inode_no));
    } else {
        return 0;
    }
}

int incremental_mark_file_for_copy(GHashTable *table, unsigned long inode_no) {
    g_hash_table_insert(table, GINT_TO_POINTER(inode_no), GINT_TO_POINTER(1));
}


#define PBSTR "========================================"
#define PBWIDTH 40

dyn_buffer_t url_escape(char *str) {

    dyn_buffer_t text = dyn_buffer_create();

    char * ptr = str;
    while (*ptr) {
        if (*ptr == '#') {
            dyn_buffer_write(&text, "%23", 3);
            ptr++;
        }

        dyn_buffer_write_char(&text, *ptr++);
    }
    dyn_buffer_write_char(&text, '\0');

    return text;
}

char *abspath(const char *path) {
    wordexp_t w;
    wordexp(path, &w, 0);

    char *abs = realpath(w.we_wordv[0], NULL);
    if (abs == NULL) {
        return NULL;
    }
    abs = realloc(abs, strlen(abs) + 2);
    strcat(abs, "/");

    wordfree(&w);
    return abs;
}

char *expandpath(const char *path) {
    wordexp_t w;
    wordexp(path, &w, 0);

    char * expanded = malloc(strlen(w.we_wordv[0]) + 2);
    strcpy(expanded, w.we_wordv[0]);
    strcat(expanded, "/");

    wordfree(&w);
    return expanded;
}

void progress_bar_print(double percentage, size_t tn_size, size_t index_size) {

    static int last_val = -1;
    int val = (int) (percentage * 100);
    if (last_val == val || val > 100 || index_size < 1024) {
        return;
    }
    last_val = val;

    int lpad = (int) ((percentage + 0.01) * PBWIDTH);
    int rpad = PBWIDTH - lpad;

    char tn_unit;
    if (tn_size > 1000 * 1000 * 1000) {
        tn_size = tn_size / 1000 / 1000 / 1000;
        tn_unit = 'G';
    } else {
        tn_size = tn_size / 1000 / 1000;
        tn_unit = 'M';
    }

    char index_unit;
    if (index_size > 1000 * 1000 * 1000) {
        index_size = index_size / 1000 / 1000 / 1000;
        index_unit = 'G';
    } else {
        index_size = index_size / 1000 / 1000;
        index_unit = 'M';
    }

    printf(
            "\r%3d%%[%.*s>%*s] TN:%3d%c IDX:%3d%c",
            val, lpad, PBSTR, rpad, "",
            (int) tn_size, tn_unit,
            (int) index_size, index_unit
    );
    fflush(stdout);
}

GHashTable *incremental_get_table() {
    GHashTable *file_table = g_hash_table_new(g_direct_hash, g_direct_equal);
    return file_table;
}


