#include "database.h"
#include "src/ctx.h"

void database_fts_attach(database_t *db, const char *fts_database_path) {

    LOG_DEBUGF("database_fts.c", "Attaching to %s", fts_database_path);

    sqlite3_stmt *stmt;
    CRASH_IF_NOT_SQLITE_OK(sqlite3_prepare_v2(
            db->db, "ATTACH DATABASE ? AS fts"
                    "", -1, &stmt, NULL));

    sqlite3_bind_text(stmt, 1, fts_database_path, -1, SQLITE_STATIC);

    CRASH_IF_STMT_FAIL(sqlite3_step(stmt));
    sqlite3_finalize(stmt);
}

void database_fts_index(database_t *db) {

    LOG_INFO("database_fts.c", "Creating content table.");

    CRASH_IF_NOT_SQLITE_OK(sqlite3_exec(
            db->db,
            "WITH docs AS (SELECT document.id                                                 as id,\n"
            "                     (SELECT id FROM descriptor)                                 as index_id,\n"
            "                     size,\n"
            "                     document.json_data ->> 'path'                               as path,\n"
            "                     length(document.json_data->>'path') - length(REPLACE(document.json_data->>'path', '/', '')) as path_depth,\n"
            "                     document.json_data ->> 'mime'                               as mime,\n"
            "                     mtime,\n"
            "                  CASE\n"
            "                         WHEN sc.json_data IS NULL THEN CASE\n"
            "                                                            WHEN t.tag IS NULL THEN json_set(\n"
            "                                                                    document.json_data, '$._id',\n"
            "                                                                    document.id, '$.size',\n"
            "                                                                    document.size, '$.mtime',\n"
            "                                                                    document.mtime)\n"
            "                                                            ELSE json_set(document.json_data, '$._id',\n"
            "                                                                          document.id, '$.size',\n"
            "                                                                          document.size, '$.mtime',\n"
            "                                                                          document.mtime, '$.tag',\n"
            "                                                                          json_group_array(t.tag)) END\n"
            "                         ELSE CASE\n"
            "                                  WHEN t.tag IS NULL THEN json_patch(\n"
            "                                          json_set(document.json_data, '$._id', document.id, '$.size',\n"
            "                                                   document.size, '$.mtime', document.mtime),\n"
            "                                          sc.json_data)\n"
            "                                  ELSE json_set(json_patch(document.json_data, sc.json_data), '$._id',\n"
            "                                                document.id, '$.size', document.size, '$.mtime',\n"
            "                                                document.mtime, '$.tag',\n"
            "                                                json_group_array(t.tag)) END END as json_data\n"
            "              FROM document\n"
            "                  LEFT JOIN document_sidecar sc ON document.id = sc.id\n"
            "                  LEFT JOIN tag t ON document.id = t.id\n"
            "              GROUP BY document.id)\n"
            "INSERT\n"
            "INTO fts.document_index (id, index_id, size, path, path_depth, mtime, mime, json_data)\n"
            "SELECT *\n"
            "FROM docs\n"
            "WHERE true\n"
            "on conflict (id, index_id) do update set size=excluded.size,\n"
            "                                         mtime=excluded.mtime,\n"
            "                                         json_data=excluded.json_data;",
            NULL, NULL, NULL));

    CRASH_IF_NOT_SQLITE_OK(sqlite3_exec(
            db->db,
            "DELETE\n"
            "FROM fts.document_index\n"
            "WHERE id IN (SELECT id FROM delete_list)\n"
            "  AND index_id = (SELECT id FROM descriptor);",
            NULL, NULL, NULL
    ));
}

void database_fts_optimize(database_t *db) {
    LOG_INFO("database_fts.c", "Optimizing search index.");

    CRASH_IF_NOT_SQLITE_OK(sqlite3_exec(
            db->db,
            "INSERT INTO search(search) VALUES('optimize');",
            NULL, NULL, NULL));
    LOG_DEBUG("database_fts.c", "Optimized fts5 table.");

    CRASH_IF_NOT_SQLITE_OK(sqlite3_exec(db->db, "PRAGMA fts.optimize;", NULL, NULL, NULL));
    LOG_DEBUG("database_fts.c", "optimized indices.");
}
