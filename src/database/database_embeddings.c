#include <openblas/cblas.h>
#include "database.h"
#include "src/ctx.h"


static float cosine_sim(int n, const float *a, const float *b) {
    float dot_product = cblas_sdot(n, a, 1, b, 1);
    float norm_a = cblas_snrm2(n, a, 1);
    float norm_b = cblas_snrm2(n, b, 1);

    return dot_product / (norm_a * norm_b);
}


void cosine_sim_func(sqlite3_context *ctx, int argc, sqlite3_value **argv) {
    if (argc != 3) {
        sqlite3_result_error(ctx, "Invalid parameters", -1);
    }

    int n = sqlite3_value_int(argv[0]);
    const float *a = sqlite3_value_blob(argv[1]);
    const float *b = sqlite3_value_blob(argv[2]);

    if (a == NULL || b == NULL) {
        sqlite3_result_double(ctx, -1);
        return;
    }

    float result = cosine_sim(n, a, b);
    if (result != result) {
        result = -1;
    }

    sqlite3_result_double(ctx, result);
}

cJSON *database_get_models(database_t *db) {
    cJSON *json = cJSON_CreateArray();
    sqlite3_stmt *stmt = db->get_models;

    int ret;
    do {
        ret = sqlite3_step(stmt);
        if (ret == SQLITE_BUSY) {
            // Database is busy (probably scanning)
            LOG_WARNING("database_embeddings.c",
                         "Database is busy, could not fetch list of models");
            break;
        }

        if (ret == SQLITE_DONE) {
            break;
        }

        cJSON *row = cJSON_CreateObject();

        cJSON_AddNumberToObject(row, "id", sqlite3_column_int(stmt, 0));
        cJSON_AddStringToObject(row, "name", (const char *) sqlite3_column_text(stmt, 1));
        cJSON_AddStringToObject(row, "url", (const char *) sqlite3_column_text(stmt, 2));
        cJSON_AddStringToObject(row, "path", (const char *) sqlite3_column_text(stmt, 3));
        cJSON_AddNumberToObject(row, "size", sqlite3_column_int(stmt, 4));
        cJSON_AddStringToObject(row, "type", (const char *) sqlite3_column_text(stmt, 5));

        cJSON_AddItemToArray(json, row);
    } while (TRUE);

    sqlite3_reset(stmt);

    return json;
}

cJSON *database_get_embedding(database_t *db, int doc_id, int model_id) {

    sqlite3_bind_int(db->get_embedding, 1, doc_id);
    sqlite3_bind_int(db->get_embedding, 2, model_id);
    int ret = sqlite3_step(db->get_embedding);
    CRASH_IF_STMT_FAIL(ret);

    if (ret == SQLITE_DONE) {
        sqlite3_reset(db->get_embedding);
        return NULL;
    }

    float *embedding = (float *) sqlite3_column_blob(db->get_embedding, 0);
    size_t size = sqlite3_column_bytes(db->get_embedding, 0) / sizeof(float);

    cJSON *json = cJSON_CreateFloatArray(embedding, (int) size);
    sqlite3_reset(db->get_embedding);

    return json;
}

void emb_to_json_func(sqlite3_context *ctx, int argc, sqlite3_value **argv) {
    if (argc != 1) {
        sqlite3_result_error(ctx, "Invalid parameters", -1);
    }

    float *embedding = (float *) sqlite3_value_blob(argv[0]);
    int size = sqlite3_value_bytes(argv[0]) / 4;

    cJSON *json = cJSON_CreateFloatArray(embedding, size);
    char *json_str = cJSON_PrintUnformatted(json);

    sqlite3_result_text(ctx, json_str, -1, SQLITE_TRANSIENT);
    free(json_str);
    cJSON_Delete(json);
}
