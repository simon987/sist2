#ifndef SIST2_STORE_H
#define SIST2_STORE_H

#include <pthread.h>
#include <lmdb.h>

#include <glib.h>

#define STORE_SIZE_TN 1024 * 1024 * 5
#define STORE_SIZE_TAG 1024 * 16
#define STORE_SIZE_META STORE_SIZE_TAG

typedef struct store_t {
    MDB_dbi dbi;
    MDB_env *env;
    size_t size;
    size_t chunk_size;
    pthread_rwlock_t lock;
} store_t;

store_t *store_create(char *path, size_t chunk_size);

void store_destroy(store_t *store);

void store_write(store_t *store, char *key, size_t key_len, char *buf, size_t buf_len);

char *store_read(store_t *store, char *key, size_t key_len, size_t *ret_vallen);

GHashTable *store_read_all(store_t *store);

void store_copy(store_t *store, const char *destination);

#endif
