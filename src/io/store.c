#include "store.h"
#include "src/ctx.h"

store_t *store_create(char *path, size_t chunk_size) {

    store_t *store = malloc(sizeof(struct store_t));
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
    ScanCtx.stat_tn_size = 0;
    mdb_env_set_mapsize(store->env, store->size);

    // Open dbi
    MDB_txn *txn;
    mdb_txn_begin(store->env, NULL, 0, &txn);
    mdb_dbi_open(txn, NULL, 0, &store->dbi);
    mdb_txn_commit(txn);

    return store;
}

void store_destroy(store_t *store) {

    pthread_rwlock_destroy(&store->lock);
    mdb_close(store->env, store->dbi);
    mdb_env_close(store->env);
    free(store);
}

void store_write(store_t *store, char *key, size_t key_len, char *buf, size_t buf_len) {

    if (LogCtx.very_verbose) {
        if (key_len == 16) {
            char uuid_str[UUID_STR_LEN] = {0, };
            uuid_unparse((unsigned char *) key, uuid_str);
            LOG_DEBUGF("store.c", "Store write {%s} %lu bytes", uuid_str, buf_len)
        } else {
            LOG_DEBUGF("store.c", "Store write {%s} %lu bytes", key, buf_len)
        }
    }

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

    if (put_ret == MDB_MAP_FULL) {
        mdb_txn_abort(txn);
        pthread_rwlock_unlock(&store->lock);

        // Cannot resize when there is a opened transaction.
        //  Resize take effect on the next commit.
        pthread_rwlock_wrlock(&store->lock);
        store->size += store->chunk_size;
        mdb_env_set_mapsize(store->env, store->size);
        mdb_txn_begin(store->env, NULL, 0, &txn);
        put_ret = mdb_put(txn, store->dbi, &mdb_key, &mdb_value, 0);

        LOG_INFOF("store.c", "Updated mdb mapsize to %lu bytes", store->size)
    }

    mdb_txn_commit(txn);
    pthread_rwlock_unlock(&store->lock);

    if (put_ret != 0) {
        LOG_ERROR("store.c", mdb_strerror(put_ret))
    }
}

char *store_read(store_t *store, char *key, size_t key_len, size_t *ret_vallen) {
    char *buf = NULL;
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
