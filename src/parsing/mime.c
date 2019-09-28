#include "mime.h"

unsigned int mime_get_mime_by_ext(GHashTable *ext_table, const char * ext) {
    char lower[64];
    char *p = lower;
    while ((*ext)) {
        *p++ = (char)tolower(*ext++);
    }
    *p = '\0';
    return (size_t) g_hash_table_lookup(ext_table, lower);
}

unsigned int mime_get_mime_by_string(GHashTable *mime_table, const char * str) {

    const char * ptr = str;
    while (*ptr == ' ' || *ptr == '[') {
        ptr++;
    }
    return (size_t) g_hash_table_lookup(mime_table, ptr);
}
