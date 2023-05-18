#include <sqlite3ext.h>
#include <string.h>
#include <stdlib.h>

SQLITE_EXTENSION_INIT1

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
    char parent[4096 * 3];
    strncpy(parent, value, stop);

    sqlite3_result_text(ctx, parent, stop, SQLITE_TRANSIENT);
}

void random_func(sqlite3_context *ctx, int argc, sqlite3_value **argv) {
    if (argc != 1 || sqlite3_value_type(argv[0]) != SQLITE_INTEGER) {
        sqlite3_result_error(ctx, "Invalid parameters", -1);
    }

    char state_buf[32] = {0,};
    struct random_data buf;
    int result;

    long seed = sqlite3_value_int64(argv[0]);

    initstate_r((int) seed, state_buf, sizeof(state_buf), &buf);

    random_r(&buf, &result);

    sqlite3_result_int(ctx, result);
}


int sqlite3_extension_init(
        sqlite3 *db,
        char **pzErrMsg,
        const sqlite3_api_routines *pApi
) {
    SQLITE_EXTENSION_INIT2(pApi);


    sqlite3_create_function(
            db,
            "path_parent",
            1,
            SQLITE_UTF8,
            NULL,
            path_parent_func,
            NULL,
            NULL
    );

    sqlite3_create_function(
            db,
            "random_seeded",
            1,
            SQLITE_UTF8,
            NULL,
            random_func,
            NULL,
            NULL
    );

    return SQLITE_OK;
}