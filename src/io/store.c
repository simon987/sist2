#include <sys/mman.h>
#include "store.h"
#include "src/ctx.h"

//#define SIST_FAKE_STORE 1

void open_env(const char *path, MDB_env **env, MDB_dbi *dbi) {
    mdb_env_create(env);

    int open_ret = mdb_env_open(*env,
                                path,
                                MDB_WRITEMAP | MDB_MAPASYNC,
                                S_IRUSR | S_IWUSR
    );

    if (open_ret != 0) {
        LOG_FATALF("store.c", "Error while opening store: %s (%s)\n", mdb_strerror(open_ret), path)
    }

    MDB_txn *txn;
    mdb_txn_begin(*env, NULL, 0, &txn);
    mdb_dbi_open(txn, NULL, 0, dbi);
    mdb_txn_commit(txn);
}

store_t *store_create(const char *path, size_t chunk_size) {
    store_t *store = calloc(1, sizeof(struct store_t));
    mkdir(path, S_IWUSR | S_IRUSR | S_IXUSR);
    strcpy(store->path, path);

    MDB_env *env;
    MDB_dbi dbi;

#if (SIST_FAKE_STORE != 1)
    store->chunk_size = chunk_size;

    store->shared_memory = mmap(NULL, sizeof(*store->shm), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    store->shm = store->shared_memory;

    open_env(path, &env, &dbi);

    store->shm->size = (size_t) store->chunk_size;
    mdb_env_set_mapsize(env, store->shm->size);

    // Close, child processes will open the environment again
    mdb_env_close(env);
#endif

    return store;
}

void store_destroy(store_t *store) {

    LOG_DEBUG("store.c", "store_destroy()")
#if (SIST_FAKE_STORE != 1)
    munmap(store->shared_memory, sizeof(*store->shm));

    mdb_dbi_close(store->proc.env, store->proc.dbi);
    mdb_env_close(store->proc.env);
#endif
    free(store);
}

void store_flush(store_t *store) {
    mdb_env_sync(store->proc.env, TRUE);
}

void store_write(store_t *store, char *key, size_t key_len, char *buf, size_t buf_len) {

    ScanCtx.stat_tn_size += buf_len;

    if (LogCtx.very_verbose) {
        LOG_DEBUGF("store.c", "Store write %s@{%s} %lu bytes", store->path, key, buf_len)
    }

#if (SIST_FAKE_STORE != 1)

    if (store->proc.env == NULL) {
         open_env(store->path, &store->proc.env, &store->proc.dbi);
         LOG_DEBUGF("store.c", "Opening mdb environment %s", store->path)
    }

    MDB_val mdb_key;
    mdb_key.mv_data = key;
    mdb_key.mv_size = key_len;

    MDB_val mdb_value;
    mdb_value.mv_data = buf;
    mdb_value.mv_size = buf_len;

    MDB_txn *txn;

    int db_full = FALSE;
    int put_ret = 0;
    int should_abort_transaction = FALSE;
    int should_increase_size = TRUE;

    int begin_ret = mdb_txn_begin(store->proc.env, NULL, 0, &txn);

    if (begin_ret == MDB_MAP_RESIZED) {
        // mapsize was increased by another process. We don't need to increase the size again, but we need
        // to update the size of the environment for the current process.
        db_full = TRUE;
        should_increase_size = FALSE;
    } else {
        put_ret = mdb_put(txn, store->proc.dbi, &mdb_key, &mdb_value, 0);

        if (put_ret == MDB_MAP_FULL) {
            // Database is full, we need to increase the environment size
            db_full = TRUE;
            should_abort_transaction = TRUE;
        } else {
            int commit_ret = mdb_txn_commit(txn);

            if (commit_ret == MDB_MAP_FULL) {
                db_full = TRUE;
            }
        }
    }

    if (db_full) {
        LOG_DEBUGF("store.c", "Updating mdb mapsize to %lu bytes", store->shm->size)

        if (should_abort_transaction) {
            mdb_txn_abort(txn);
        }

        // Cannot resize when there is an opened transaction in this process.
        //  Resize take effect on the next commit.
        if (should_increase_size) {
            store->shm->size += store->chunk_size;
        }
        int resize_ret = mdb_env_set_mapsize(store->proc.env, store->shm->size);
        if (resize_ret != 0) {
            LOG_ERRORF("store.c", "mdb_env_set_mapsize() failed: %s", mdb_strerror(resize_ret))
        }
        mdb_txn_begin(store->proc.env, NULL, 0, &txn);
        int put_ret_retry = mdb_put(txn, store->proc.dbi, &mdb_key, &mdb_value, 0);

        if (put_ret_retry != 0) {
            LOG_ERRORF("store.c", "mdb_put() (retry) failed: %s", mdb_strerror(put_ret_retry))
        }

        int ret = mdb_txn_commit(txn);
        if (ret != 0) {
            LOG_FATALF("store.c", "FIXME: Could not commit to store %s: %s (%d), %d, %d %d",
                       store->path, mdb_strerror(ret), ret,
                       ret, put_ret_retry)
        }
        LOG_DEBUGF("store.c", "Updated mdb mapsize to %lu bytes", store->shm->size)
    } else if (put_ret != 0) {
        LOG_ERRORF("store.c", "mdb_put() failed: %s", mdb_strerror(put_ret))
    }

#endif
}

char *store_read(store_t *store, char *key, size_t key_len, size_t *return_value_len) {
    char *buf = NULL;

#if (SIST_FAKE_STORE != 1)
    if (store->proc.env == NULL) {
        open_env(store->path, &store->proc.env, &store->proc.dbi);
    }

    MDB_val mdb_key;
    mdb_key.mv_data = key;
    mdb_key.mv_size = key_len;

    MDB_val mdb_value;

    MDB_txn *txn;
    mdb_txn_begin(store->proc.env, NULL, MDB_RDONLY, &txn);

    int get_ret = mdb_get(txn, store->proc.dbi, &mdb_key, &mdb_value);

    if (get_ret == MDB_NOTFOUND) {
        *return_value_len = 0;
    } else {
        *return_value_len = mdb_value.mv_size;
        buf = malloc(mdb_value.mv_size);
        memcpy(buf, mdb_value.mv_data, mdb_value.mv_size);
    }

    mdb_txn_abort(txn);
#endif
    return buf;
}

GHashTable *store_read_all(store_t *store) {

    if (store->proc.env == NULL) {
        open_env(store->path, &store->proc.env, &store->proc.dbi);
        LOG_DEBUGF("store.c", "Opening mdb environment %s", store->path)
    }

    int count = 0;

    GHashTable *table = g_hash_table_new_full(g_str_hash, g_str_equal, free, free);

    MDB_txn *txn = NULL;
    mdb_txn_begin(store->proc.env, NULL, MDB_RDONLY, &txn);

    MDB_cursor *cur = NULL;
    mdb_cursor_open(txn, store->proc.dbi, &cur);

    MDB_val key;
    MDB_val value;

    while (mdb_cursor_get(cur, &key, &value, MDB_NEXT) == 0) {
        char *key_str = malloc(key.mv_size);
        memcpy(key_str, key.mv_data, key.mv_size);
        char *val_str = malloc(value.mv_size);
        memcpy(val_str, value.mv_data, value.mv_size);

        g_hash_table_insert(table, key_str, val_str);
        count += 1;
    }

    const char *path;
    mdb_env_get_path(store->proc.env, &path);
    LOG_DEBUGF("store.c", "Read %d entries from %s", count, path)

    mdb_cursor_close(cur);
    mdb_txn_abort(txn);
    return table;
}


void store_copy(store_t *store, const char *destination) {
    mkdir(destination, S_IWUSR | S_IRUSR | S_IXUSR);
    mdb_env_copy(store->proc.env, destination);
}
