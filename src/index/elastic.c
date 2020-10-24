#include "elastic.h"
#include "src/ctx.h"

#include "web.h"

#include "static_generated.c"


typedef struct es_indexer {
    int queued;
    char *es_url;
    char *es_index;
    es_bulk_line_t *line_head;
    es_bulk_line_t *line_tail;
} es_indexer_t;


static __thread es_indexer_t *Indexer;

void delete_queue(int max);

void elastic_flush();

void elastic_cleanup() {
    elastic_flush();
    if (Indexer != NULL) {
        free(Indexer->es_index);
        free(Indexer->es_url);
        free(Indexer);
    }
}

void print_json(cJSON *document, const char uuid_str[UUID_STR_LEN]) {

    cJSON *line = cJSON_CreateObject();

    cJSON_AddStringToObject(line, "_id", uuid_str);
    cJSON_AddStringToObject(line, "_index", IndexCtx.es_index);
    cJSON_AddStringToObject(line, "_type", "_doc");
    cJSON_AddItemReferenceToObject(line, "_source", document);

    char *json = cJSON_PrintUnformatted(line);

    printf("%s\n", json);

    cJSON_free(json);
    cJSON_Delete(line);
}

void index_json_func(void *arg) {
    es_bulk_line_t *line = arg;
    elastic_index_line(line);
}

void index_json(cJSON *document, const char uuid_str[UUID_STR_LEN]) {
    char *json = cJSON_PrintUnformatted(document);

    size_t json_len = strlen(json);
    es_bulk_line_t *bulk_line = malloc(sizeof(es_bulk_line_t) + json_len + 2);
    memcpy(bulk_line->line, json, json_len);
    memcpy(bulk_line->uuid_str, uuid_str, UUID_STR_LEN);
    *(bulk_line->line + json_len) = '\n';
    *(bulk_line->line + json_len + 1) = '\0';
    bulk_line->next = NULL;

    cJSON_free(json);
    tpool_add_work(IndexCtx.pool, index_json_func, bulk_line);
}

void execute_update_script(const char *script, int async, const char index_id[UUID_STR_LEN]) {

    if (Indexer == NULL) {
        Indexer = create_indexer(IndexCtx.es_url, IndexCtx.es_index);
    }

    cJSON *body = cJSON_CreateObject();
    cJSON *script_obj = cJSON_AddObjectToObject(body, "script");
    cJSON_AddStringToObject(script_obj, "lang", "painless");
    cJSON_AddStringToObject(script_obj, "source", script);

    cJSON *query = cJSON_AddObjectToObject(body, "query");
    cJSON *term_obj = cJSON_AddObjectToObject(query, "term");
    cJSON_AddStringToObject(term_obj, "index", index_id);

    char *str = cJSON_Print(body);

    char bulk_url[4096];
    if (async) {
        snprintf(bulk_url, sizeof(bulk_url), "%s/%s/_update_by_query?wait_for_completion=false", Indexer->es_url,
                 Indexer->es_index);
    } else {
        snprintf(bulk_url, sizeof(bulk_url), "%s/%s/_update_by_query", Indexer->es_url, Indexer->es_index);
    }
    response_t *r = web_post(bulk_url, str);
    if (!async) {
        LOG_INFOF("elastic.c", "Executed user script <%d>", r->status_code);
    }
    cJSON *resp = cJSON_Parse(r->body);

    cJSON_free(str);
    cJSON_Delete(body);
    free_response(r);

    cJSON *error = cJSON_GetObjectItem(resp, "error");
    if (error != NULL) {
        char *error_str = cJSON_Print(error);

        LOG_ERRORF("elastic.c", "User script error: \n%s", error_str);
        cJSON_free(error_str);
    }

    if (async) {
        cJSON *task = cJSON_GetObjectItem(resp, "task");
        LOG_INFOF("elastic.c", "User script queued: %s/_tasks/%s", Indexer->es_url, task->valuestring);
    }

    cJSON_Delete(resp);
}

void *create_bulk_buffer(int max, int *count, size_t *buf_len) {
    es_bulk_line_t *line = Indexer->line_head;
    *count = 0;

    size_t buf_size = 0;
    size_t buf_cur = 0;
    char *buf = malloc(8192);
    size_t buf_capacity = 8192;

    while (line != NULL && *count < max) {
        char action_str[256];
        snprintf(
                action_str, 256,
                "{\"index\":{\"_id\":\"%s\",\"_type\":\"_doc\",\"_index\":\"%s\"}}\n",
                line->uuid_str, Indexer->es_index
        );

        size_t action_str_len = strlen(action_str);
        size_t line_len = strlen(line->line);

        while (buf_size + line_len + action_str_len > buf_capacity) {
            buf_capacity *= 2;
            buf = realloc(buf, buf_capacity);
        }

        buf_size += line_len + action_str_len;

        memcpy(buf + buf_cur, action_str, action_str_len);
        buf_cur += action_str_len;
        memcpy(buf + buf_cur, line->line, line_len);
        buf_cur += line_len;

        line = line->next;
        (*count)++;
    }

    if (buf_size + 1 > buf_capacity) {
        buf = realloc(buf, buf_capacity + 1);
    }

    *(buf + buf_cur) = '\0';

    *buf_len = buf_cur;
    return buf;
}

void print_errors(response_t *r) {
    char *tmp = malloc(r->size + 1);
    memcpy(tmp, r->body, r->size);
    *(tmp + r->size) = '\0';

    cJSON *ret_json = cJSON_Parse(tmp);
    if (cJSON_GetObjectItem(ret_json, "errors")->valueint != 0) {
        cJSON *err;
        cJSON_ArrayForEach(err, cJSON_GetObjectItem(ret_json, "items")) {
            if (cJSON_GetObjectItem(cJSON_GetObjectItem(err, "index"), "status")->valueint != 201) {
                char *str = cJSON_Print(err);
                LOG_ERRORF("elastic.c", "%s\n", str);
                cJSON_free(str);
            }
        }
    }
    cJSON_Delete(ret_json);
    free(tmp);
}

void print_error(response_t *r) {
    char *tmp = malloc(r->size + 1);
    memcpy(tmp, r->body, r->size);
    *(tmp + r->size) = '\0';

    cJSON *ret_json = cJSON_Parse(tmp);
    if (cJSON_GetObjectItem(ret_json, "error") != NULL) {
        char *str = cJSON_Print(cJSON_GetObjectItem(ret_json, "error"));
        LOG_ERRORF("elastic.c", "%s\n", str);
        cJSON_free(str);
    }
    cJSON_Delete(ret_json);
    free(tmp);
}

void _elastic_flush(int max) {

    if (max == 0) {
        LOG_WARNING("elastic.c", "calling _elastic_flush with 0 in queue")
        return;
    }

    size_t buf_len;
    int count;
    void *buf = create_bulk_buffer(max, &count, &buf_len);

    char bulk_url[4096];
    snprintf(bulk_url, sizeof(bulk_url), "%s/%s/_bulk?pipeline=tie", Indexer->es_url, Indexer->es_index);
    response_t *r = web_post(bulk_url, buf);

    if (r->status_code == 0) {
        LOG_FATALF("elastic.c", "Could not connect to %s, make sure that elasticsearch is running!\n", IndexCtx.es_url)
    }

    if (r->status_code == 413) {

        if (max <= 1) {
            LOG_ERRORF("elastic.c", "Single document too large, giving up: {%s}", Indexer->line_head->uuid_str)
            free_response(r);
            free(buf);
            delete_queue(1);
            if (Indexer->queued != 0) {
                elastic_flush();
            }
            return;
        }

        LOG_WARNINGF("elastic.c", "Payload too large, retrying (%d documents)", count);

        free_response(r);
        free(buf);
        _elastic_flush(max / 2);
        return;

    } else if (r->status_code == 429) {

        free_response(r);
        free(buf);
        LOG_WARNING("elastic.c", "Got 429 status, will retry after delay")
        usleep(1000000 * 20);
        _elastic_flush(max);
        return;

    } else if (r->status_code != 200) {
        print_errors(r);
        delete_queue(Indexer->queued);

    } else {

        print_errors(r);
        LOG_INFOF("elastic.c", "Indexed %d documents (%zukB) <%d>", count, buf_len / 1024, r->status_code);
        delete_queue(max);

        if (Indexer->queued != 0) {
            elastic_flush();
        }
    }

    free_response(r);
    free(buf);
}

void delete_queue(int max) {
    for (int i = 0; i < max; i++) {
        es_bulk_line_t *tmp = Indexer->line_head;
        Indexer->line_head = tmp->next;
        if (Indexer->line_head == NULL) {
            Indexer->line_tail = NULL;
        }
        free(tmp);
        Indexer->queued -= 1;
    }
}

void elastic_flush() {

    if (Indexer == NULL) {
        Indexer = create_indexer(IndexCtx.es_url, IndexCtx.es_index);
    }

    _elastic_flush(Indexer->queued);
}

void elastic_index_line(es_bulk_line_t *line) {

    if (Indexer == NULL) {
        Indexer = create_indexer(IndexCtx.es_url, IndexCtx.es_index);
    }

    if (Indexer->line_head == NULL) {
        Indexer->line_head = line;
        Indexer->line_tail = Indexer->line_head;
    } else {
        Indexer->line_tail->next = line;
        Indexer->line_tail = line;
    }

    Indexer->queued += 1;

    if (Indexer->queued >= IndexCtx.batch_size) {
        elastic_flush();
    }
}

es_indexer_t *create_indexer(const char *url, const char *index) {

    char *es_url = malloc(strlen(url) + 1);
    strcpy(es_url, url);

    char *es_index = malloc(strlen(index) + 1);
    strcpy(es_index, index);

    es_indexer_t *indexer = malloc(sizeof(es_indexer_t));

    indexer->es_url = es_url;
    indexer->es_index = es_index;
    indexer->queued = 0;
    indexer->line_head = NULL;
    indexer->line_tail = NULL;

    return indexer;
}

void finish_indexer(char *script, int async_script, char *index_id) {

    char url[4096];

    snprintf(url, sizeof(url), "%s/%s/_refresh", IndexCtx.es_url, IndexCtx.es_index);
    response_t *r = web_post(url, "");
    LOG_INFOF("elastic.c", "Refresh index <%d>", r->status_code);
    free_response(r);

    if (script != NULL) {
        execute_update_script(script, async_script, index_id);
        free(script);

        snprintf(url, sizeof(url), "%s/%s/_refresh", IndexCtx.es_url, IndexCtx.es_index);
        r = web_post(url, "");
        LOG_INFOF("elastic.c", "Refresh index <%d>", r->status_code);
        free_response(r);
    }

    snprintf(url, sizeof(url), "%s/%s/_forcemerge", IndexCtx.es_url, IndexCtx.es_index);
    r = web_post(url, "");
    LOG_INFOF("elastic.c", "Merge index <%d>", r->status_code);
    free_response(r);

    snprintf(url, sizeof(url), "%s/%s/_settings", IndexCtx.es_url, IndexCtx.es_index);
    r = web_put(url, "{\"index\":{\"refresh_interval\":\"1s\"}}");
    LOG_INFOF("elastic.c", "Set refresh interval <%d>", r->status_code);
    free_response(r);
}

void elastic_init(int force_reset, const char* user_mappings, const char* user_settings) {

    // Check if index exists
    char url[4096];
    snprintf(url, sizeof(url), "%s/%s", IndexCtx.es_url, IndexCtx.es_index);
    response_t *r = web_get(url, 30);
    int index_exists = r->status_code == 200;
    free_response(r);

    if (!index_exists || force_reset) {
        r = web_delete(url);
        LOG_INFOF("elastic.c", "Delete index <%d>", r->status_code);
        free_response(r);

        snprintf(url, sizeof(url), "%s/%s", IndexCtx.es_url, IndexCtx.es_index);
        r = web_put(url, "");

        if (r->status_code != 200) {
            print_error(r);
            LOG_FATAL("elastic.c", "Could not create index")
        }

        LOG_INFOF("elastic.c", "Create index <%d>", r->status_code);
        free_response(r);

        snprintf(url, sizeof(url), "%s/%s/_close", IndexCtx.es_url, IndexCtx.es_index);
        r = web_post(url, "");
        LOG_INFOF("elastic.c", "Close index <%d>", r->status_code);
        free_response(r);

        snprintf(url, sizeof(url), "%s/_ingest/pipeline/tie", IndexCtx.es_url);
        r = web_put(url, pipeline_json);
        LOG_INFOF("elastic.c", "Create pipeline <%d>", r->status_code);
        free_response(r);

        snprintf(url, sizeof(url), "%s/%s/_settings", IndexCtx.es_url, IndexCtx.es_index);
        r = web_put(url, user_settings ? user_settings : settings_json);
        LOG_INFOF("elastic.c", "Update user_settings <%d>", r->status_code);
        free_response(r);

        snprintf(url, sizeof(url), "%s/%s/_mappings/_doc?include_type_name=true", IndexCtx.es_url, IndexCtx.es_index);
        r = web_put(url, user_mappings ? user_mappings : mappings_json);
        LOG_INFOF("elastic.c", "Update user_mappings <%d>", r->status_code);
        free_response(r);

        snprintf(url, sizeof(url), "%s/%s/_open", IndexCtx.es_url, IndexCtx.es_index);
        r = web_post(url, "");
        LOG_INFOF("elastic.c", "Open index <%d>", r->status_code);
        free_response(r);
    }
}

cJSON *elastic_get_document(const char *uuid_str) {
    char url[4096];
    snprintf(url, sizeof(url), "%s/%s/_doc/%s", WebCtx.es_url, WebCtx.es_index, uuid_str);

    response_t *r = web_get(url, 3);
    cJSON *json = NULL;
    if (r->status_code == 200) {
        char *tmp = malloc(r->size + 1);
        memcpy(tmp, r->body, r->size);
        *(tmp + r->size) = '\0';
        json = cJSON_Parse(tmp);
        free(tmp);
    }
    free_response(r);
    return json;
}

char *elastic_get_status() {
    char url[4096];
    snprintf(url, sizeof(url),
             "%s/_cluster/state/metadata/%s?filter_path=metadata.indices.*.state", WebCtx.es_url, WebCtx.es_index);

    response_t *r = web_get(url, 30);
    cJSON *json = NULL;
    char *status = malloc(128 * sizeof(char));
    status[0] = '\0';

    if (r->status_code == 200) {
        char *tmp = malloc(r->size + 1);
        memcpy(tmp, r->body, r->size);
        *(tmp + r->size) = '\0';
        json = cJSON_Parse(tmp);
        free(tmp);
        const cJSON *metadata = cJSON_GetObjectItem(json, "metadata");
        if (metadata != NULL) {
            const cJSON *indices = cJSON_GetObjectItem(metadata, "indices");
            const cJSON *index = cJSON_GetObjectItem(indices, WebCtx.es_index);
            const cJSON *state = cJSON_GetObjectItem(index, "state");
            strcpy(status, state->valuestring);
        }
    }
    free_response(r);
    cJSON_Delete(json);
    return status;
}
