#ifndef SIST2_STORE_H
#define SIST2_STORE_H

#include <pthread.h>
#include <lmdb.h>

typedef struct store_t {
    MDB_dbi dbi;
    MDB_env *env;
    size_t size;
    pthread_rwlock_t lock;
} store_t;

store_t *store_create(char *path);

void store_destroy(store_t *store);

void store_write(store_t *store, char *key, size_t key_len, char *buf, size_t buf_len);

char *store_read(store_t *store, char *key, size_t key_len, size_t *ret_vallen);

#endif
