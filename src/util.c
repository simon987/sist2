#include "util.h"
#include "src/ctx.h"

#define PBSTR "========================================"
#define PBWIDTH 40

dyn_buffer_t url_escape(char *str) {

    dyn_buffer_t text = dyn_buffer_create();

    char *ptr = str;
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
    if (strlen(abs) > 1) {
        abs = realloc(abs, strlen(abs) + 2);
        strcat(abs, "/");
    }

    wordfree(&w);
    return abs;
}

char *expandpath(const char *path) {
    wordexp_t w;
    wordexp(path, &w, 0);

    char *expanded = malloc(strlen(w.we_wordv[0]) + 2);
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

const char *find_file_in_paths(const char *paths[], const char *filename) {

    for (int i = 0; paths[i] != NULL; i++) {

        char *apath = abspath(paths[i]);
        if (apath == NULL) {
            continue;
        }

        char path[PATH_MAX];
        snprintf(path, sizeof(path), "%s%s", apath, filename);

        LOG_DEBUGF("util.c", "Looking for '%s' in folder '%s'", filename, apath)
        free(apath);

        struct stat info;
        int ret = stat(path, &info);
        if (ret != -1) {
            return paths[i];
        }
    }

    return NULL;
}


