#ifndef SIST2_UTIL_H
#define SIST2_UTIL_H

#include <stdio.h>

#define TEXT_BUF_FULL -1
#define INITIAL_BUF_SIZE 1024 * 16

#define SHOULD_IGNORE_CHAR(c) !(SHOULD_KEEP_CHAR(c))
#define SHOULD_KEEP_CHAR(c) (c >= (int)'!')


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

dyn_buffer_t dyn_buffer_create();

void grow_buffer(dyn_buffer_t *buf, size_t size);

void grow_buffer_small(dyn_buffer_t *buf);

void dyn_buffer_write(dyn_buffer_t *buf, void *data, size_t size);

void dyn_buffer_write_char(dyn_buffer_t *buf, char c);

void dyn_buffer_write_str(dyn_buffer_t *buf, char *str);

void dyn_buffer_append_string(dyn_buffer_t *buf, char *str);

void dyn_buffer_write_int(dyn_buffer_t *buf, int d);

void dyn_buffer_write_short(dyn_buffer_t *buf, short s);

void dyn_buffer_write_long(dyn_buffer_t *buf, unsigned long l);

void dyn_buffer_destroy(dyn_buffer_t *buf);

void text_buffer_destroy(text_buffer_t *buf);

text_buffer_t text_buffer_create(int max_size);

void text_buffer_terminate_string(text_buffer_t *buf);

int text_buffer_append_string(text_buffer_t *buf, char *str, size_t len);
int text_buffer_append_string0(text_buffer_t *buf, char *str);

int text_buffer_append_char(text_buffer_t *buf, int c);

void incremental_put(GHashTable *table, unsigned long inode_no, int mtime);

int incremental_get(GHashTable *table, unsigned long inode_no);

int incremental_mark_file_for_copy(GHashTable *table, unsigned long inode_no);


#endif
