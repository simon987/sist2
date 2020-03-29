#ifndef SIST2_UTIL_H
#define SIST2_UTIL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h>

#include "third-party/utf8.h/utf8.h"
#include "libscan/scan.h"



char *abspath(const char *path);

char *expandpath(const char *path);

dyn_buffer_t url_escape(char *str);

void progress_bar_print(double percentage, size_t tn_size, size_t index_size);

GHashTable *incremental_get_table();

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
    return g_hash_table_insert(table, GINT_TO_POINTER(inode_no), GINT_TO_POINTER(1));
}


const char *find_file_in_paths(const char **paths, const char *filename);

#endif
