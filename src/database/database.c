#include "database.h"
#include "malloc.h"
#include "src/ctx.h"
#include <string.h>
#include <pthread.h>
#include "src/util.h"

#include <time.h>


database_t *database_create(const char *filename, database_type_t type) {
    database_t *db = malloc(sizeof(database_t));

    strcpy(db->filename, filename);
    db->type = type;
    db->select_thumbnail_stmt = NULL;

    db->ipc_ctx = NULL;

    return db;
}

__always_inline
static int sep_rfind(const char *str) {
    for (int i = (int) strlen(str); i >= 0; i--) {
        if (str[i] == '/') {
            return i;
        }
    }
    return -1;
}

void path_parent_func(sqlite3_context *ctx, int argc, sqlite3_value **argv) {
    if (argc != 1 || sqlite3_value_type(argv[0]) != SQLITE_TEXT) {
        sqlite3_result_error(ctx, "Invalid parameters", -1);
    }

    const char *value = (const char *) sqlite3_value_text(argv[0]);

    int stop = sep_rfind(value);
    if (stop == -1) {
        sqlite3_result_null(ctx);
        return;
    }
    char parent[PATH_MAX * 3];
    strncpy(parent, value, stop);

    sqlite3_result_text(ctx, parent, stop, SQLITE_TRANSIENT);
}


void save_current_job_info(sqlite3_context *ctx, int argc, sqlite3_value **argv) {
    if (argc != 1 || sqlite3_value_type(argv[0]) != SQLITE_TEXT) {
        sqlite3_result_error(ctx, "Invalid parameters", -1);
    }

    database_ipc_ctx_t *ipc_ctx = sqlite3_user_data(ctx);

    const char *current_job = (const char *) sqlite3_value_text(argv[0]);

    char buf[PATH_MAX];
    strcpy(buf, current_job);

    strcpy(ipc_ctx->current_job[ProcData.thread_id], current_job);

    sqlite3_result_text(ctx, "ok", -1, SQLITE_STATIC);
}

void database_initialize(database_t *db) {
    CRASH_IF_NOT_SQLITE_OK(sqlite3_open(db->filename, &db->db));

    LOG_DEBUGF("database.c", "Initializing database %s", db->filename);
    if (db->type == INDEX_DATABASE) {
        CRASH_IF_NOT_SQLITE_OK(sqlite3_exec(db->db, IndexDatabaseSchema, NULL, NULL, NULL));
    } else if (db->type == IPC_CONSUMER_DATABASE || db->type == IPC_PRODUCER_DATABASE) {
        CRASH_IF_NOT_SQLITE_OK(sqlite3_exec(db->db, IpcDatabaseSchema, NULL, NULL, NULL));
    }

    sqlite3_close(db->db);
}

void database_open(database_t *db) {
    LOG_DEBUGF("database.c", "Opening database %s (%d)", db->filename, db->type);

    CRASH_IF_NOT_SQLITE_OK(sqlite3_open(db->filename, &db->db));
    sqlite3_busy_timeout(db->db, 1000);

    CRASH_IF_NOT_SQLITE_OK(sqlite3_exec(db->db, "PRAGMA cache_size = -200000;", NULL, NULL, NULL));
    CRASH_IF_NOT_SQLITE_OK(sqlite3_exec(db->db, "PRAGMA synchronous = OFF;", NULL, NULL, NULL));

    if (db->type == INDEX_DATABASE) {
        CRASH_IF_NOT_SQLITE_OK(sqlite3_exec(db->db, "PRAGMA temp_store = memory;", NULL, NULL, NULL));
    }

    if (db->type == INDEX_DATABASE) {
        // Prepare statements;
        CRASH_IF_NOT_SQLITE_OK(sqlite3_prepare_v2(
                db->db,
                "SELECT data FROM thumbnail WHERE id=? AND num=? LIMIT 1;", -1,
                &db->select_thumbnail_stmt, NULL));
        CRASH_IF_NOT_SQLITE_OK(sqlite3_prepare_v2(
                db->db,
                "UPDATE document SET marked=1 WHERE id=? AND mtime=? RETURNING id",
                -1,
                &db->mark_document_stmt, NULL));
        CRASH_IF_NOT_SQLITE_OK(sqlite3_prepare_v2(
                db->db,
                "REPLACE INTO document_sidecar (id, json_data) VALUES (?,?)", -1,
                &db->write_document_sidecar_stmt, NULL));
        CRASH_IF_NOT_SQLITE_OK(sqlite3_prepare_v2(
                db->db,
                "REPLACE INTO document (id, mtime, size, json_data) VALUES (?, ?, ?, ?);", -1,
                &db->write_document_stmt, NULL));
        CRASH_IF_NOT_SQLITE_OK(sqlite3_prepare_v2(
                db->db,
                "INSERT INTO thumbnail (id, num, data) VALUES (?,?,?) ON CONFLICT DO UPDATE SET data=excluded.data;",
                -1,
                &db->write_thumbnail_stmt, NULL));

        // Create functions
        sqlite3_create_function(
                db->db,
                "path_parent",
                1,
                SQLITE_UTF8,
                NULL,
                path_parent_func,
                NULL,
                NULL
        );
    } else if (db->type == IPC_CONSUMER_DATABASE) {

        sqlite3_create_function(
                db->db,
                "save_current_job_info",
                1,
                SQLITE_UTF8,
                db->ipc_ctx,
                save_current_job_info,
                NULL,
                NULL
        );

        CRASH_IF_NOT_SQLITE_OK(sqlite3_prepare_v2(
                db->db,
                "DELETE FROM parse_job WHERE id = (SELECT MIN(id) FROM parse_job)"
                " RETURNING filepath,mtime,st_size,save_current_job_info(filepath);",
                -1, &db->pop_parse_job_stmt, NULL
        ));
        CRASH_IF_NOT_SQLITE_OK(sqlite3_prepare_v2(
                db->db,
                "DELETE FROM index_job WHERE id = (SELECT MIN(id) FROM index_job)"
                " RETURNING doc_id,type,line;",
                -1, &db->pop_index_job_stmt, NULL
        ));

    } else if (db->type == IPC_PRODUCER_DATABASE) {
        char sql[40];
        int max_size_mb = 10; // TODO: read from args.

        snprintf(sql, sizeof(sql), "PRAGMA max_page_count=%d", (max_size_mb * 1024 * 1024) / 4096);
        CRASH_IF_NOT_SQLITE_OK(sqlite3_exec(db->db, sql, NULL, NULL, NULL));

        CRASH_IF_NOT_SQLITE_OK(sqlite3_prepare_v2(
                db->db, "INSERT INTO parse_job (filepath,mtime,st_size) VALUES (?,?,?);", -1,
                &db->insert_parse_job_stmt, NULL));
        CRASH_IF_NOT_SQLITE_OK(sqlite3_prepare_v2(
                db->db, "INSERT INTO index_job (doc_id,type,line) VALUES (?,?,?);", -1,
                &db->insert_index_job_stmt, NULL));

        sqlite3_create_function(
                db->db,
                "path_parent",
                1,
                SQLITE_UTF8,
                NULL,
                path_parent_func,
                NULL,
                NULL
        );
    }

}

void database_close(database_t *db, int optimize) {
    LOG_DEBUGF("database.c", "Closing database %s", db->filename);

    if (optimize) {
        LOG_DEBUG("database.c", "Optimizing database");
        CRASH_IF_NOT_SQLITE_OK(sqlite3_exec(db->db, "VACUUM;", NULL, NULL, NULL));
        CRASH_IF_NOT_SQLITE_OK(sqlite3_exec(db->db, "PRAGMA optimize;", NULL, NULL, NULL));
    }

    sqlite3_close(db->db);

    if (db->type == IPC_PRODUCER_DATABASE) {
        remove(db->filename);
    }

    free(db);
    db = NULL;
}

void *database_read_thumbnail(database_t *db, const char *id, int num, size_t *return_value_len) {
    sqlite3_bind_text(db->select_thumbnail_stmt, 1, id, -1, SQLITE_STATIC);
    sqlite3_bind_int(db->select_thumbnail_stmt, 2, num);

    int ret = sqlite3_step(db->select_thumbnail_stmt);

    if (ret == SQLITE_DONE) {
        CRASH_IF_NOT_SQLITE_OK(sqlite3_reset(db->select_thumbnail_stmt));
        *return_value_len = 0;
        return NULL;
    }

    CRASH_IF_STMT_FAIL(ret);

    const void *blob = sqlite3_column_blob(db->select_thumbnail_stmt, 0);
    const int blob_size = sqlite3_column_bytes(db->select_thumbnail_stmt, 0);

    *return_value_len = blob_size;
    void *return_data = malloc(blob_size);
    memcpy(return_data, blob, blob_size);

    CRASH_IF_NOT_SQLITE_OK(sqlite3_reset(db->select_thumbnail_stmt));

    return return_data;
}

void database_write_index_descriptor(database_t *db, index_descriptor_t *desc) {

    sqlite3_exec(db->db, "DELETE FROM descriptor;", NULL, NULL, NULL);

    sqlite3_stmt *stmt;

    sqlite3_prepare_v2(db->db, "INSERT INTO descriptor (id, version_major, version_minor, version_patch,"
                               " root, name, rewrite_url, timestamp) VALUES (?,?,?,?,?,?,?,?);", -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, desc->id, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, desc->version_major);
    sqlite3_bind_int(stmt, 3, desc->version_minor);
    sqlite3_bind_int(stmt, 4, desc->version_patch);
    sqlite3_bind_text(stmt, 5, desc->root, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 6, desc->name, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 7, desc->rewrite_url, -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 8, desc->timestamp);

    CRASH_IF_STMT_FAIL(sqlite3_step(stmt));

    sqlite3_finalize(stmt);
}

index_descriptor_t *database_read_index_descriptor(database_t *db) {

    sqlite3_stmt *stmt;

    sqlite3_prepare_v2(db->db, "SELECT id, version_major, version_minor, version_patch,"
                               " root, name, rewrite_url, timestamp FROM descriptor;", -1, &stmt, NULL);

    CRASH_IF_STMT_FAIL(sqlite3_step(stmt));

    const char *id = (char *) sqlite3_column_text(stmt, 0);
    int v_major = sqlite3_column_int(stmt, 1);
    int v_minor = sqlite3_column_int(stmt, 2);
    int v_patch = sqlite3_column_int(stmt, 3);
    const char *root = (char *) sqlite3_column_text(stmt, 4);
    const char *name = (char *) sqlite3_column_text(stmt, 5);
    const char *rewrite_url = (char *) sqlite3_column_text(stmt, 6);
    int timestamp = sqlite3_column_int(stmt, 7);

    index_descriptor_t *desc = malloc(sizeof(index_descriptor_t));
    strcpy(desc->id, id);
    snprintf(desc->version, sizeof(desc->version), "%d.%d.%d", v_major, v_minor, v_patch);
    desc->version_major = v_major;
    desc->version_minor = v_minor;
    desc->version_patch = v_patch;
    strcpy(desc->root, root);
    strcpy(desc->name, name);
    strcpy(desc->rewrite_url, rewrite_url);
    desc->timestamp = timestamp;

    CRASH_IF_NOT_SQLITE_OK(sqlite3_finalize(stmt));

    return desc;
}

database_iterator_t *database_create_delete_list_iterator(database_t *db) {

    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db->db, "SELECT id FROM delete_list;", -1, &stmt, NULL);

    database_iterator_t *iter = malloc(sizeof(database_iterator_t));

    iter->stmt = stmt;
    iter->db = db;

    return iter;
}

char *database_delete_list_iter(database_iterator_t *iter) {
    int ret = sqlite3_step(iter->stmt);

    if (ret == SQLITE_ROW) {
        const char *id = (const char *) sqlite3_column_text(iter->stmt, 0);
        char *id_heap = malloc(strlen(id) + 1);
        strcpy(id_heap, id);
        return id_heap;
    }

    if (ret != SQLITE_DONE) {
        LOG_FATALF("database.c", "FIXME: delete iter returned %s", sqlite3_errmsg(iter->db->db));
    }

    if (sqlite3_finalize(iter->stmt) != SQLITE_OK) {
        LOG_FATALF("database.c", "FIXME: delete iter returned %s", sqlite3_errmsg(iter->db->db));
    }

    iter->stmt = NULL;

    return NULL;
}

database_iterator_t *database_create_document_iterator(database_t *db) {

    sqlite3_stmt *stmt;

    // TODO optimization: remove mtime, size, _id from json_data

    sqlite3_prepare_v2(db->db, "WITH doc (j) AS (SELECT CASE"
                               " WHEN sc.json_data IS NULL THEN"
                               "  CASE"
                               "   WHEN t.tag IS NULL THEN"
                               "    json_set(document.json_data, '$._id', document.id, '$.size', document.size, '$.mtime', document.mtime)"
                               "   ELSE"
                               "    json_set(document.json_data, '$._id', document.id, '$.size', document.size, '$.mtime', document.mtime, '$.tag', json_group_array(t.tag))"
                               "   END"
                               " ELSE"
                               "  CASE"
                               "   WHEN t.tag IS NULL THEN"
                               "    json_patch(json_set(document.json_data, '$._id', document.id, '$.size', document.size, '$.mtime', document.mtime), sc.json_data)"
                               "   ELSE"
                               //   This will overwrite any tags specified in the sidecar file!
                               //   TODO: concatenate the two arrays?
                               "    json_set(json_patch(document.json_data, sc.json_data), '$._id', document.id, '$.size', document.size, '$.mtime', document.mtime, '$.tag', json_group_array(t.tag))"
                               "   END"
                               " END"
                               " FROM document"
                               " LEFT JOIN document_sidecar sc ON document.id = sc.id"
                               " LEFT JOIN tag t ON document.id = t.id"
                               " GROUP BY document.id)"
                               " SELECT json_set(j, '$.index', (SELECT id FROM descriptor)) FROM doc", -1, &stmt, NULL);

    database_iterator_t *iter = malloc(sizeof(database_iterator_t));

    iter->stmt = stmt;
    iter->db = db;

    return iter;
}

cJSON *database_document_iter(database_iterator_t *iter) {

    if (iter->stmt == NULL) {
        LOG_ERROR("database.c", "FIXME: database_document_iter() called after iteration stopped");
        return NULL;
    }

    int ret = sqlite3_step(iter->stmt);

    if (ret == SQLITE_ROW) {
        const char *json_string = (const char *) sqlite3_column_text(iter->stmt, 0);
        return cJSON_Parse(json_string);
    }

    if (ret != SQLITE_DONE) {
        LOG_FATALF("database.c", "FIXME: doc iter returned %s", sqlite3_errmsg(iter->db->db));
    }

    if (sqlite3_finalize(iter->stmt) != SQLITE_OK) {
        LOG_FATALF("database.c", "FIXME: doc iter returned %s", sqlite3_errmsg(iter->db->db));
    }

    iter->stmt = NULL;

    return NULL;
}

cJSON *database_incremental_scan_begin(database_t *db) {
    LOG_DEBUG("database.c", "Preparing database for incremental scan");
    CRASH_IF_NOT_SQLITE_OK(sqlite3_exec(db->db, "UPDATE document SET marked=0;", NULL, NULL, NULL));
}

cJSON *database_incremental_scan_end(database_t *db) {
    CRASH_IF_NOT_SQLITE_OK(sqlite3_exec(
            db->db,
            "DELETE FROM delete_list WHERE id IN (SELECT id FROM document WHERE marked=1);",
            NULL, NULL, NULL
    ));

    CRASH_IF_NOT_SQLITE_OK(sqlite3_exec(
            db->db,
            "DELETE FROM thumbnail WHERE id IN (SELECT id FROM document WHERE marked=0);",
            NULL, NULL, NULL
    ));

    CRASH_IF_NOT_SQLITE_OK(sqlite3_exec(
            db->db,
            "INSERT INTO delete_list (id) SELECT id FROM document WHERE marked=0;",
            NULL, NULL, NULL
    ));

    CRASH_IF_NOT_SQLITE_OK(sqlite3_exec(
            db->db,
            "DELETE FROM document_sidecar WHERE id IN (SELECT id FROM document WHERE marked=0);",
            NULL, NULL, NULL
    ));

    CRASH_IF_NOT_SQLITE_OK(sqlite3_exec(
            db->db,
            "DELETE FROM document WHERE marked=0;",
            NULL, NULL, NULL
    ));
}

int database_mark_document(database_t *db, const char *id, int mtime) {
    sqlite3_bind_text(db->mark_document_stmt, 1, id, -1, SQLITE_STATIC);
    sqlite3_bind_int(db->mark_document_stmt, 2, mtime);

    pthread_mutex_lock(&db->ipc_ctx->index_db_mutex);
    int ret = sqlite3_step(db->mark_document_stmt);

    if (ret == SQLITE_ROW) {
        CRASH_IF_NOT_SQLITE_OK(sqlite3_reset(db->mark_document_stmt));
        pthread_mutex_unlock(&db->ipc_ctx->index_db_mutex);
        return TRUE;
    }

    if (ret == SQLITE_DONE) {
        CRASH_IF_NOT_SQLITE_OK(sqlite3_reset(db->mark_document_stmt));
        pthread_mutex_unlock(&db->ipc_ctx->index_db_mutex);
        return FALSE;
    }
    pthread_mutex_unlock(&db->ipc_ctx->index_db_mutex);

    CRASH_IF_STMT_FAIL(ret);
}

void database_write_document(database_t *db, document_t *doc, const char *json_data) {
    sqlite3_bind_text(db->write_document_stmt, 1, doc->doc_id, -1, SQLITE_STATIC);
    sqlite3_bind_int(db->write_document_stmt, 2, doc->mtime);
    sqlite3_bind_int64(db->write_document_stmt, 3, (long) doc->size);
    sqlite3_bind_text(db->write_document_stmt, 4, json_data, -1, SQLITE_STATIC);

    pthread_mutex_lock(&db->ipc_ctx->index_db_mutex);
    CRASH_IF_STMT_FAIL(sqlite3_step(db->write_document_stmt));
    CRASH_IF_NOT_SQLITE_OK(sqlite3_reset(db->write_document_stmt));
    pthread_mutex_unlock(&db->ipc_ctx->index_db_mutex);
}


void database_write_document_sidecar(database_t *db, const char *id, const char *json_data) {
    sqlite3_bind_text(db->write_document_sidecar_stmt, 1, id, -1, SQLITE_STATIC);
    sqlite3_bind_text(db->write_document_sidecar_stmt, 2, json_data, -1, SQLITE_STATIC);

    pthread_mutex_lock(&db->ipc_ctx->index_db_mutex);
    CRASH_IF_STMT_FAIL(sqlite3_step(db->write_document_sidecar_stmt));
    CRASH_IF_NOT_SQLITE_OK(sqlite3_reset(db->write_document_sidecar_stmt));
    pthread_mutex_unlock(&db->ipc_ctx->index_db_mutex);
}

void database_write_thumbnail(database_t *db, const char *id, int num, void *data, size_t data_size) {
    sqlite3_bind_text(db->write_thumbnail_stmt, 1, id, -1, SQLITE_STATIC);
    sqlite3_bind_int(db->write_thumbnail_stmt, 2, num);
    sqlite3_bind_blob(db->write_thumbnail_stmt, 3, data, (int) data_size, SQLITE_STATIC);

    pthread_mutex_lock(&db->ipc_ctx->index_db_mutex);
    CRASH_IF_STMT_FAIL(sqlite3_step(db->write_thumbnail_stmt));
    CRASH_IF_NOT_SQLITE_OK(sqlite3_reset(db->write_thumbnail_stmt));
    pthread_mutex_unlock(&db->ipc_ctx->index_db_mutex);
}


//void database_create_fts_index(database_t *db, database_t *fts_db) {
//    // In a separate file,
//
//    // use database_initialize() to create FTS schema
//    // if --force-reset, then truncate the tables first
//
//    /*
//     * create/append fts table
//     *
//     * create/append scalar index table with
//     *  id,index,size,mtime,mime
//     *
//     * create/append path index table with
//     *  index,path,depth
//     *
//     * content table is a view with SELECT UNION for all attached tables
//     *  random_seed column
//     */
//
//    // INSERT INTO ft(ft) VALUES('optimize');
//}

job_t *database_get_work(database_t *db, job_type_t job_type) {
    job_t *job;

    pthread_mutex_lock(&db->ipc_ctx->mutex);
    while (db->ipc_ctx->job_count == 0 && !db->ipc_ctx->no_more_jobs) {
        pthread_cond_timedwait_ms(&db->ipc_ctx->has_work_cond, &db->ipc_ctx->mutex, 10);
    }
    pthread_mutex_unlock(&db->ipc_ctx->mutex);

    pthread_mutex_lock(&db->ipc_ctx->db_mutex);

    if (job_type == JOB_PARSE_JOB) {
        int ret = sqlite3_step(db->pop_parse_job_stmt);
        if (ret == SQLITE_DONE) {
            CRASH_IF_NOT_SQLITE_OK(sqlite3_reset(db->pop_parse_job_stmt));
            pthread_mutex_unlock(&db->ipc_ctx->db_mutex);
            return NULL;
        } else {
            CRASH_IF_STMT_FAIL(ret);
        }

        job = malloc(sizeof(*job));

        job->parse_job = create_parse_job(
                (const char *) sqlite3_column_text(db->pop_parse_job_stmt, 0),
                sqlite3_column_int(db->pop_parse_job_stmt, 1),
                sqlite3_column_int64(db->pop_parse_job_stmt, 2));

        CRASH_IF_NOT_SQLITE_OK(sqlite3_reset(db->pop_parse_job_stmt));
    } else {

        int ret = sqlite3_step(db->pop_index_job_stmt);

        if (ret == SQLITE_DONE) {
            CRASH_IF_NOT_SQLITE_OK(sqlite3_reset(db->pop_index_job_stmt));
            pthread_mutex_unlock(&db->ipc_ctx->db_mutex);
            return NULL;
        }

        CRASH_IF_STMT_FAIL(ret);

        job = malloc(sizeof(*job));

        const char *line = (const char *) sqlite3_column_text(db->pop_index_job_stmt, 2);
        if (line != NULL) {
            job->bulk_line = malloc(sizeof(es_bulk_line_t) + strlen(line) + 1);
            strcpy(job->bulk_line->line, line);
        } else {
            job->bulk_line = malloc(sizeof(es_bulk_line_t));
        }
        strcpy(job->bulk_line->doc_id, (const char *) sqlite3_column_text(db->pop_index_job_stmt, 0));
        job->bulk_line->type = sqlite3_column_int(db->pop_index_job_stmt, 1);
        job->bulk_line->next = NULL;

        CRASH_IF_NOT_SQLITE_OK(sqlite3_reset(db->pop_index_job_stmt));
    }

    pthread_mutex_unlock(&db->ipc_ctx->db_mutex);

    pthread_mutex_lock(&db->ipc_ctx->mutex);
    db->ipc_ctx->job_count -= 1;
    pthread_mutex_unlock(&db->ipc_ctx->mutex);

    job->type = job_type;
    return job;
}

void database_add_work(database_t *db, job_t *job) {
    int ret;

    pthread_mutex_lock(&db->ipc_ctx->db_mutex);

    if (job->type == JOB_PARSE_JOB) {
        do {
            sqlite3_bind_text(db->insert_parse_job_stmt, 1, job->parse_job->filepath, -1, SQLITE_STATIC);
            sqlite3_bind_int(db->insert_parse_job_stmt, 2, job->parse_job->vfile.mtime);
            sqlite3_bind_int64(db->insert_parse_job_stmt, 3, (long) job->parse_job->vfile.st_size);

            ret = sqlite3_step(db->insert_parse_job_stmt);

            if (ret == SQLITE_FULL) {
                sqlite3_reset(db->insert_parse_job_stmt);
                pthread_mutex_unlock(&db->ipc_ctx->db_mutex);
                usleep(1000000);
                pthread_mutex_lock(&db->ipc_ctx->db_mutex);
                continue;
            } else {
                CRASH_IF_STMT_FAIL(ret);
            }

            ret = sqlite3_reset(db->insert_parse_job_stmt);
            if (ret == SQLITE_FULL) {
                pthread_mutex_unlock(&db->ipc_ctx->db_mutex);
                usleep(100000);
                pthread_mutex_lock(&db->ipc_ctx->db_mutex);
            } else if (ret != SQLITE_OK) {
                LOG_FATALF("database.c", "sqlite3_reset returned error %d", ret);
            }
        } while (ret != SQLITE_DONE && ret != SQLITE_OK);
    } else if (job->type == JOB_BULK_LINE) {
        do {
            sqlite3_bind_text(db->insert_index_job_stmt, 1, job->bulk_line->doc_id, -1, SQLITE_STATIC);
            sqlite3_bind_int(db->insert_index_job_stmt, 2, job->bulk_line->type);
            if (job->bulk_line->type != ES_BULK_LINE_DELETE) {
                sqlite3_bind_text(db->insert_index_job_stmt, 3, job->bulk_line->line, -1, SQLITE_STATIC);
            } else {
                sqlite3_bind_null(db->insert_index_job_stmt, 3);
            }

            ret = sqlite3_step(db->insert_index_job_stmt);

            if (ret == SQLITE_FULL) {
                sqlite3_reset(db->insert_index_job_stmt);
                pthread_mutex_unlock(&db->ipc_ctx->db_mutex);
                usleep(100000);
                pthread_mutex_lock(&db->ipc_ctx->db_mutex);
                continue;
            } else {
                CRASH_IF_STMT_FAIL(ret);
            }

            ret = sqlite3_reset(db->insert_index_job_stmt);
            if (ret == SQLITE_FULL) {
                pthread_mutex_unlock(&db->ipc_ctx->db_mutex);
                usleep(100000);
                pthread_mutex_lock(&db->ipc_ctx->db_mutex);
            } else if (ret != SQLITE_OK) {
                LOG_FATALF("database.c", "sqlite3_reset returned error %d", ret);
            }

        } while (ret != SQLITE_DONE && ret != SQLITE_OK);
    } else {
        LOG_FATAL("database.c", "FIXME: invalid job type");
    }
    pthread_mutex_unlock(&db->ipc_ctx->db_mutex);

    pthread_mutex_lock(&db->ipc_ctx->mutex);
    db->ipc_ctx->job_count += 1;
    pthread_cond_signal(&db->ipc_ctx->has_work_cond);
    pthread_mutex_unlock(&db->ipc_ctx->mutex);
}
