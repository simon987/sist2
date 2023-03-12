#ifndef SIST2_STORE_H
#define SIST2_STORE_H

#include <pthread.h>
#include <lmdb.h>

#include <glib.h>

#define STORE_SIZE_TN (1024 * 1024 * 5)
#define STORE_SIZE_TAG (1024 * 1024)
#define STORE_SIZE_META STORE_SIZE_TAG


typedef struct store_t {
    char path[PATH_MAX];
    size_t chunk_size;

    struct {
        MDB_dbi dbi;
        MDB_env *env;
    } proc;

    struct {
        size_t size;
    } *shm;
} store_t;

store_t *store_create(const char *path, size_t chunk_size);

void store_destroy(store_t *store);

void store_write(store_t *store, char *key, size_t key_len, char *buf, size_t buf_len);

void store_flush(store_t *store);

char *store_read(store_t *store, char *key, size_t key_len, size_t *return_value_len);

GHashTable *store_read_all(store_t *store);

void store_copy(store_t *store, const char *destination);

#endif
