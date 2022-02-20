#include "store.h"
#include "src/ctx.h"

store_t *store_create(const char *path, size_t chunk_size) {
    store_t *store = malloc(sizeof(struct store_t));
    mkdir(path, S_IWUSR | S_IRUSR | S_IXUSR);
    strcpy(store->path, path);

#if (SIST_FAKE_STORE != 1)
    store->chunk_size = chunk_size;
    pthread_rwlock_init(&store->lock, NULL);

    mdb_env_create(&store->env);

    int open_ret = mdb_env_open(store->env,
                                path,
                                MDB_WRITEMAP | MDB_MAPASYNC,
                                S_IRUSR | S_IWUSR
    );

    if (open_ret != 0) {
        LOG_FATALF("store.c", "Error while opening store: %s (%s)\n", mdb_strerror(open_ret), path)
    }

    store->size = (size_t) store->chunk_size;
    mdb_env_set_mapsize(store->env, store->size);

    // Open dbi
    MDB_txn *txn;
    mdb_txn_begin(store->env, NULL, 0, &txn);
    mdb_dbi_open(txn, NULL, 0, &store->dbi);
    mdb_txn_commit(txn);
#endif

    return store;
}

void store_destroy(store_t *store) {

#if (SIST_FAKE_STORE != 1)
    pthread_rwlock_destroy(&store->lock);
    mdb_dbi_close(store->env, store->dbi);
    mdb_env_close(store->env);
#endif
    free(store);
}

void store_flush(store_t *store) {
    mdb_env_sync(store->env, TRUE);
}

void store_write(store_t *store, char *key, size_t key_len, char *buf, size_t buf_len) {

    if (LogCtx.very_verbose) {
        if (key_len == MD5_DIGEST_LENGTH) {
            char path_md5_str[MD5_STR_LENGTH];
            buf2hex((unsigned char *) key, MD5_DIGEST_LENGTH, path_md5_str);

            LOG_DEBUGF("store.c", "Store write {%s} %lu bytes", path_md5_str, buf_len)

        } else if (key_len == MD5_DIGEST_LENGTH + sizeof(int)) {
            char path_md5_str[MD5_STR_LENGTH];
            buf2hex((unsigned char *) key, MD5_DIGEST_LENGTH, path_md5_str);

            LOG_DEBUGF("store.c", "Store write {%s/%d} %lu bytes",
                       path_md5_str, *(int *) (key + MD5_DIGEST_LENGTH), buf_len);

        } else {
            LOG_DEBUGF("store.c", "Store write {%s} %lu bytes", key, buf_len)
        }
    }

#if (SIST_FAKE_STORE != 1)

    MDB_val mdb_key;
    mdb_key.mv_data = key;
    mdb_key.mv_size = key_len;

    MDB_val mdb_value;
    mdb_value.mv_data = buf;
    mdb_value.mv_size = buf_len;

    MDB_txn *txn;
    pthread_rwlock_rdlock(&store->lock);
    mdb_txn_begin(store->env, NULL, 0, &txn);

    int put_ret = mdb_put(txn, store->dbi, &mdb_key, &mdb_value, 0);
    ScanCtx.stat_tn_size += buf_len;

    int db_full = FALSE;
    int should_abort_transaction = FALSE;

    if (put_ret == MDB_MAP_FULL) {
        db_full = TRUE;
        should_abort_transaction = TRUE;
    } else {
        int commit_ret = mdb_txn_commit(txn);

        if (commit_ret == MDB_MAP_FULL) {
            db_full = TRUE;
        }
    }

    if (db_full) {
        LOG_INFOF("store.c", "Updating mdb mapsize to %lu bytes", store->size)

        if (should_abort_transaction) {
            mdb_txn_abort(txn);
        }

        pthread_rwlock_unlock(&store->lock);

        // Cannot resize when there is a opened transaction.
        //  Resize take effect on the next commit.
        pthread_rwlock_wrlock(&store->lock);
        store->size += store->chunk_size;
        int resize_ret = mdb_env_set_mapsize(store->env, store->size);
        if (resize_ret != 0) {
            LOG_ERROR("store.c", mdb_strerror(put_ret))
        }
        mdb_txn_begin(store->env, NULL, 0, &txn);
        int put_ret_retry = mdb_put(txn, store->dbi, &mdb_key, &mdb_value, 0);

        if (put_ret_retry != 0) {
            LOG_ERROR("store.c", mdb_strerror(put_ret))
        }

        int ret = mdb_txn_commit(txn);
        if (ret != 0) {
            LOG_FATALF("store.c", "FIXME: Could not commit to store %s: %s (%d), %d, %d %d",
                       store->path, mdb_strerror(ret), ret,
                       put_ret, put_ret_retry);
        }
        LOG_INFOF("store.c", "Updated mdb mapsize to %lu bytes", store->size)
    } else if (put_ret != 0) {
        LOG_ERROR("store.c", mdb_strerror(put_ret))
    }

    pthread_rwlock_unlock(&store->lock);

#endif
}

char *store_read(store_t *store, char *key, size_t key_len, size_t *ret_vallen) {
    char *buf = NULL;

#if (SIST_FAKE_STORE != 1)
    MDB_val mdb_key;
    mdb_key.mv_data = key;
    mdb_key.mv_size = key_len;

    MDB_val mdb_value;

    MDB_txn *txn;
    mdb_txn_begin(store->env, NULL, MDB_RDONLY, &txn);

    int get_ret = mdb_get(txn, store->dbi, &mdb_key, &mdb_value);

    if (get_ret == MDB_NOTFOUND) {
        *ret_vallen = 0;
    } else {
        *ret_vallen = mdb_value.mv_size;
        buf = malloc(mdb_value.mv_size);
        memcpy(buf, mdb_value.mv_data, mdb_value.mv_size);
    }

    mdb_txn_abort(txn);
#endif
    return buf;
}

GHashTable *store_read_all(store_t *store) {

    int count = 0;

    GHashTable *table = g_hash_table_new_full(g_str_hash, g_str_equal, free, free);

    MDB_txn *txn = NULL;
    mdb_txn_begin(store->env, NULL, MDB_RDONLY, &txn);

    MDB_cursor *cur = NULL;
    mdb_cursor_open(txn, store->dbi, &cur);

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
    mdb_env_get_path(store->env, &path);
    LOG_DEBUGF("store.c", "Read %d entries from %s", count, path);

    mdb_cursor_close(cur);
    mdb_txn_abort(txn);
    return table;
}


void store_copy(store_t *store, const char *destination) {
    mkdir(destination, S_IWUSR | S_IRUSR | S_IXUSR);
    mdb_env_copy(store->env, destination);
}
