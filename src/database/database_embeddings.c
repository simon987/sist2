#include <openblas/cblas.h>
#include "database.h"


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

void embedding_to_json_func(sqlite3_context *ctx, int argc, sqlite3_value **argv) {

    // emb, type, start, end, size

    if (argc != 5) {
        sqlite3_result_error(ctx, "Invalid parameters", -1);
    }

    const float *embedding = sqlite3_value_blob(argv[0]);
    const char *type = (const char *) sqlite3_value_text(argv[1]);

    int size = sqlite3_value_int(argv[4]);

    if (strcmp(type, "flat") == 0) {

        cJSON *json = cJSON_CreateFloatArray(embedding, size);

        char *json_str = cJSON_PrintBuffered(json, size * 22, FALSE);

        cJSON_Delete(json);

        sqlite3_result_text(ctx, json_str, -1, SQLITE_TRANSIENT);
        free(json_str);

    } else {
        int start = sqlite3_value_int(argv[2]);
        int end = sqlite3_value_int(argv[3]);

        sqlite3_result_error(ctx, "Nested embeddings not implemented yet", -1);
    }
}

cJSON *database_get_models(database_t *db) {
    cJSON *json = cJSON_CreateArray();
    sqlite3_stmt *stmt = db->get_models;

    int ret;
    do {
        ret = sqlite3_step(stmt);
        CRASH_IF_STMT_FAIL(ret);

        if (ret == SQLITE_DONE) {
            break;
        }

        cJSON *row = cJSON_CreateObject();

        cJSON_AddNumberToObject(row, "id", sqlite3_column_int(stmt, 0));
        cJSON_AddStringToObject(row, "name", (const char *) sqlite3_column_text(stmt, 1));
        cJSON_AddStringToObject(row, "url", (const char *) sqlite3_column_int64(stmt, 2));
        cJSON_AddStringToObject(row, "path", (const char *) sqlite3_column_text(stmt, 3));
        cJSON_AddNumberToObject(row, "size", sqlite3_column_int(stmt, 4));
        cJSON_AddStringToObject(row, "type", (const char *) sqlite3_column_text(stmt, 5));

        cJSON_AddItemToArray(json, row);
    } while (TRUE);

    return json;
}
