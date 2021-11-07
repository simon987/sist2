#include "util.h"
#include "src/ctx.h"

#include <wordexp.h>

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

    char *expanded = expandpath(path);

    char *abs = realpath(expanded, NULL);
    free(expanded);
    if (abs == NULL) {
        return NULL;
    }
    if (strlen(abs) > 1) {
        abs = realloc(abs, strlen(abs) + 2);
        strcat(abs, "/");
    }

    return abs;
}

void shell_escape(char *dst, const char *src) {
    const char *ptr = src;
    char *out = dst;
    while ((*ptr)) {
        char c = *ptr++;

        if (c == '&' || c == '\n' || c == '|' || c == ';' || c == '<' ||
            c == '>' || c == '(' || c == ')' || c == '{' || c == '}') {
            *out++ = '\\';
        }
        *out++ = c;
    }
    *out = 0;
}

char *expandpath(const char *path) {
    char tmp[PATH_MAX * 2];

    shell_escape(tmp, path);

    wordexp_t w;
    wordexp(tmp, &w, 0);

    if (w.we_wordv == NULL) {
        return NULL;
    }

    *tmp = '\0';
    for (int i = 0; i < w.we_wordc; i++) {
        strcat(tmp, w.we_wordv[i]);
        if (i != w.we_wordc - 1) {
            strcat(tmp, " ");
        }
    }

    char *expanded = malloc(strlen(tmp) + 2);
    strcpy(expanded, tmp);
    strcat(expanded, "/");

    wordfree(&w);
    return expanded;
}

void progress_bar_print(double percentage, size_t tn_size, size_t index_size) {

    static int last_val = -1;
    int val = (int) (percentage * 100);
    if (last_val == val || val > 100) {
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
    GHashTable *file_table = g_hash_table_new_full(g_str_hash, g_str_equal, free, NULL);
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

#define ESCAPE_CHAR ']'

void str_escape(char *dst, const char *str) {
    const size_t len = strlen(str);

    char buf[16384];
    memset(buf + len, 0, 8);
    strcpy(buf, str);

    char *cur = dst;
    const char *ptr = buf;
    const char *oldPtr = ptr;

    utf8_int32_t c;
    char tmp[16];

    do {
        ptr = (char *) utf8codepoint(ptr, &c);
        *(int *) tmp = 0x00000000;
        size_t code_len = (ptr - oldPtr);
        memcpy(tmp, oldPtr, code_len);
        oldPtr = ptr;

        if (!utf8_validchr2(tmp)) {
            for (int i = 0; i < code_len; i++) {
                if (tmp[i] == 0) {
                    break;
                }

                cur += sprintf(cur, "%c%02X", ESCAPE_CHAR, (unsigned char) tmp[i]);
            }
            continue;
        }

        if (c == ESCAPE_CHAR) {
            *cur++ = ESCAPE_CHAR;
            *cur++ = ESCAPE_CHAR;
            continue;
        }

        if (((utf8_int32_t) 0xffffff80 & c) == 0) {
            *(cur++) = (char) c;
        } else if (((utf8_int32_t) 0xfffff800 & c) == 0) {
            *(cur++) = 0xc0 | (char) (c >> 6);
            *(cur++) = 0x80 | (char) (c & 0x3f);
        } else if (((utf8_int32_t) 0xffff0000 & c) == 0) {
            *(cur++) = 0xe0 | (char) (c >> 12);
            *(cur++) = 0x80 | (char) ((c >> 6) & 0x3f);
            *(cur++) = 0x80 | (char) (c & 0x3f);
        } else {
            *(cur++) = 0xf0 | (char) (c >> 18);
            *(cur++) = 0x80 | (char) ((c >> 12) & 0x3f);
            *(cur++) = 0x80 | (char) ((c >> 6) & 0x3f);
            *(cur++) = 0x80 | (char) (c & 0x3f);
        }

    } while (*ptr != '\0');

    *cur = '\0';
}

void str_unescape(char *dst, const char *str) {
    char *cur = dst;
    const char *ptr = str;

    char tmp[3];
    tmp[2] = '\0';

    while (*ptr != 0) {
        char c = *ptr++;

        if (c == ESCAPE_CHAR) {
            char next = *ptr;

            if (next == ESCAPE_CHAR) {
                *cur++ = (char) c;
                ptr += 1;
            } else {
                tmp[0] = *(ptr);
                tmp[1] = *(ptr + 1);
                *cur++ = (char) strtol(tmp, NULL, 16);
                ptr += 2;
            }
        } else {
            *cur++ = c;
        }
    }
    *cur = '\0';
}
