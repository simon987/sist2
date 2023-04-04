#ifndef SIST2_DATABASE_H
#define SIST2_DATABASE_H

#include <sqlite3.h>
#include <cjson/cJSON.h>
#include "src/sist.h"
#include "src/index/elastic.h"

typedef struct index_descriptor index_descriptor_t;

extern const char *IpcDatabaseSchema;
extern const char *IndexDatabaseSchema;

typedef enum {
    INDEX_DATABASE,
    IPC_CONSUMER_DATABASE,
    IPC_PRODUCER_DATABASE,
    FTS_DATABASE
} database_type_t;

typedef enum {
    JOB_UNDEFINED,
    JOB_BULK_LINE,
    JOB_PARSE_JOB
} job_type_t;

typedef struct {
    job_type_t type;
    union {
        parse_job_t *parse_job;
        es_bulk_line_t *bulk_line;
    };
} job_t;

typedef struct {
    int job_count;
    int no_more_jobs;
    int completed_job_count;

    pthread_mutex_t mutex;
    pthread_mutex_t db_mutex;
    pthread_mutex_t index_db_mutex;
    pthread_cond_t has_work_cond;
    char current_job[256][PATH_MAX * 2];
} database_ipc_ctx_t;

typedef struct database {
    char filename[PATH_MAX];
    database_type_t type;
    sqlite3 *db;

    // Prepared statements
    sqlite3_stmt *select_thumbnail_stmt;
    sqlite3_stmt *treemap_merge_up_update_stmt;
    sqlite3_stmt *treemap_merge_up_delete_stmt;

    sqlite3_stmt *mark_document_stmt;
    sqlite3_stmt *write_document_stmt;
    sqlite3_stmt *write_document_sidecar_stmt;
    sqlite3_stmt *write_thumbnail_stmt;

    sqlite3_stmt *insert_parse_job_stmt;
    sqlite3_stmt *insert_index_job_stmt;
    sqlite3_stmt *pop_parse_job_stmt;
    sqlite3_stmt *pop_index_job_stmt;

    database_ipc_ctx_t *ipc_ctx;
} database_t;

typedef struct {
    database_t *db;
    sqlite3_stmt *stmt;
} database_iterator_t;

typedef struct {
    const char *path;
    const char *parent;
    long size;
} treemap_row_t;

static treemap_row_t null_treemap_row = {0, 0, 0};


database_t *database_create(const char *filename, database_type_t type);

void database_initialize(database_t *db);

void database_open(database_t *db);

void database_close(database_t *, int optimize);

void database_write_thumbnail(database_t *db, const char *id, int num, void *data, size_t data_size);

void *database_read_thumbnail(database_t *db, const char *id, int num, size_t *return_value_len);

void database_write_index_descriptor(database_t *db, index_descriptor_t *desc);

index_descriptor_t *database_read_index_descriptor(database_t *db);

void database_write_document(database_t *db, document_t *doc, const char *json_data);

database_iterator_t *database_create_document_iterator(database_t *db);

cJSON *database_document_iter(database_iterator_t *);

#define database_document_iter_foreach(element, iter) \
    for (cJSON *element = database_document_iter(iter); element != NULL; element = database_document_iter(iter))

cJSON *database_incremental_scan_begin(database_t *db);

cJSON *database_incremental_scan_end(database_t *db);

int database_mark_document(database_t *db, const char *id, int mtime);

void database_write_document_sidecar(database_t *db, const char *id, const char *json_data);

database_iterator_t *database_create_treemap_iterator(database_t *db, long threshold);

treemap_row_t database_treemap_iter(database_iterator_t *iter);

#define database_treemap_iter_foreach(element, iter) \
    for (treemap_row_t element = database_treemap_iter(iter); element.path != NULL; element = database_treemap_iter(iter))


void database_generate_stats(database_t *db, double treemap_threshold);

job_t *database_get_work(database_t *db, job_type_t job_type);

void database_add_work(database_t *db, job_t *job);

//void database_index(database_t *db);

#define CRASH_IF_STMT_FAIL(x) do { \
        int return_value = x;                \
        if (return_value != SQLITE_DONE && return_value != SQLITE_ROW) {     \
            LOG_FATALF("database.c", "Sqlite error @ database.c:%d : (%d) %s", __LINE__, return_value, sqlite3_errmsg(db->db)); \
        }                           \
    } while (0)

#define CRASH_IF_NOT_SQLITE_OK(x) do { \
        int return_value = x;                \
        if (return_value != SQLITE_OK) {     \
            LOG_FATALF("database.c", "Sqlite error @ database.c:%d : (%d) %s", __LINE__, return_value, sqlite3_errmsg(db->db)); \
        }                           \
    } while (0)

#endif //SIST2_DATABASE_H