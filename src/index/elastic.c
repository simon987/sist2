#include "elastic.h"
#include "src/ctx.h"

#include "web.h"

#include "static_generated.c"


typedef struct es_indexer {
    int queued;
    char *es_url;
    es_bulk_line_t *line_head;
    es_bulk_line_t *line_tail;
} es_indexer_t;


static __thread es_indexer_t *Indexer;

void delete_queue(int max);
void elastic_flush();

void elastic_cleanup() {
    elastic_flush();
    if (Indexer != NULL) {
        free(Indexer->es_url);
        free(Indexer);
    }
}

void print_json(cJSON *document, const char uuid_str[UUID_STR_LEN]) {

    cJSON *line = cJSON_CreateObject();

    cJSON_AddStringToObject(line, "_id", uuid_str);
    cJSON_AddStringToObject(line, "_index", "sist2");
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

void execute_update_script(const char *script, const char index_id[UUID_STR_LEN]) {

    if (Indexer == NULL) {
        Indexer = create_indexer(IndexCtx.es_url);
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
    snprintf(bulk_url, 4096, "%s/sist2/_update_by_query?wait_for_completion=false", Indexer->es_url);
    response_t *r = web_post(bulk_url, str);
    LOG_INFOF("elastic.c", "Executed user script <%d>", r->status_code);
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

    cJSON_Delete(resp);
}

#define ACTION_STR_LEN 91

void *create_bulk_buffer(int max, int *count, size_t *buf_len) {
    es_bulk_line_t *line = Indexer->line_head;
    *count = 0;

    size_t buf_size = 0;
    size_t buf_cur = 0;
    char *buf = malloc(8196);
    size_t buf_capacity = 8196;

    while (line != NULL && *count < max) {
        char action_str[256];
        snprintf(action_str, 256,
                 "{\"index\":{\"_id\":\"%s\", \"_type\":\"_doc\", \"_index\":\"sist2\"}}\n", line->uuid_str);

        size_t line_len = strlen(line->line);

        while (buf_size + line_len + ACTION_STR_LEN > buf_capacity) {
            buf_capacity *= 2;
            buf = realloc(buf, buf_capacity);
        }

        buf_size += line_len + ACTION_STR_LEN;

        memcpy(buf + buf_cur, action_str, ACTION_STR_LEN);
        buf_cur += ACTION_STR_LEN;
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

void _elastic_flush(int max) {

    if (max == 0) {
        LOG_WARNING("elastic.c", "calling _elastic_flush with 0 in queue")
        return;
    }

    size_t buf_len;
    int count;
    void *buf = create_bulk_buffer(max, &count, &buf_len);

    char bulk_url[4096];
    snprintf(bulk_url, 4096, "%s/sist2/_bulk?pipeline=tie", Indexer->es_url);
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
        Indexer = create_indexer(IndexCtx.es_url);
    }

    _elastic_flush(Indexer->queued);
}

void elastic_index_line(es_bulk_line_t *line) {

    if (Indexer == NULL) {
        Indexer = create_indexer(IndexCtx.es_url);
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

es_indexer_t *create_indexer(const char *url) {

    char *es_url = malloc(strlen(url) + 1);
    strcpy(es_url, url);

    es_indexer_t *indexer = malloc(sizeof(es_indexer_t));

    indexer->es_url = es_url;
    indexer->queued = 0;
    indexer->line_head = NULL;
    indexer->line_tail = NULL;

    return indexer;
}

void finish_indexer(char *script, char *index_id) {

    char url[4096];

    snprintf(url, sizeof(url), "%s/sist2/_refresh", IndexCtx.es_url);
    response_t *r = web_post(url, "");
    LOG_INFOF("elastic.c", "Refresh index <%d>", r->status_code);
    free_response(r);

    if (script != NULL) {
        execute_update_script(script, index_id);
        free(script);

        snprintf(url, sizeof(url), "%s/sist2/_refresh", IndexCtx.es_url);
        r = web_post(url, "");
        LOG_INFOF("elastic.c", "Refresh index <%d>", r->status_code);
        free_response(r);
    }

    snprintf(url, sizeof(url), "%s/sist2/_forcemerge", IndexCtx.es_url);
    r = web_post(url, "");
    LOG_INFOF("elastic.c", "Merge index <%d>", r->status_code);
    free_response(r);

    snprintf(url, sizeof(url), "%s/sist2/_settings", IndexCtx.es_url);
    r = web_put(url, "{\"index\":{\"refresh_interval\":\"1s\"}}");
    LOG_INFOF("elastic.c", "Set refresh interval <%d>", r->status_code);
    free_response(r);
}

void elastic_init(int force_reset) {

    // Check if index exists
    char url[4096];
    snprintf(url, 4096, "%s/sist2", IndexCtx.es_url);
    response_t *r = web_get(url, 30);
    int index_exists = r->status_code == 200;
    free_response(r);

    if (!index_exists || force_reset) {
        r = web_delete(url);
        LOG_INFOF("elastic.c", "Delete index <%d>", r->status_code);
        free_response(r);

        snprintf(url, 4096, "%s/sist2", IndexCtx.es_url);
        r = web_put(url, "");
        LOG_INFOF("elastic.c", "Create index <%d>", r->status_code);
        free_response(r);

        snprintf(url, 4096, "%s/sist2/_close", IndexCtx.es_url);
        r = web_post(url, "");
        LOG_INFOF("elastic.c", "Close index <%d>", r->status_code);
        free_response(r);

        snprintf(url, 4096, "%s/_ingest/pipeline/tie", IndexCtx.es_url);
        r = web_put(url, pipeline_json);
        LOG_INFOF("elastic.c", "Create pipeline <%d>", r->status_code);
        free_response(r);

        snprintf(url, 4096, "%s/sist2/_settings", IndexCtx.es_url);
        r = web_put(url, settings_json);
        LOG_INFOF("elastic.c", "Update settings <%d>", r->status_code);
        free_response(r);

        snprintf(url, 4096, "%s/sist2/_mappings/_doc?include_type_name=true", IndexCtx.es_url);
        r = web_put(url, mappings_json);
        LOG_INFOF("elastic.c", "Update mappings <%d>", r->status_code);
        free_response(r);

        snprintf(url, 4096, "%s/sist2/_open", IndexCtx.es_url);
        r = web_post(url, "");
        LOG_INFOF("elastic.c", "Open index <%d>", r->status_code);
        free_response(r);
    }
}

cJSON *elastic_get_document(const char *uuid_str) {
    char url[4096];
    snprintf(url, 4096, "%s/sist2/_doc/%s", WebCtx.es_url, uuid_str);

    response_t *r = web_get(url, 3);
    cJSON *json = NULL;
    if (r->status_code == 200) {
        json = cJSON_Parse(r->body);
    }
    free_response(r);
    return json;
}

char *elastic_get_status() {
    char url[4096];
    snprintf(url, 4096,
             "%s/_cluster/state/metadata/sist2?filter_path=metadata.indices.*.state", WebCtx.es_url);

    response_t *r = web_get(url, 30);
    cJSON *json = NULL;
    char *status = malloc(128 * sizeof(char));
    status[0] = '\0';

    if (r->status_code == 200) {
        json = cJSON_Parse(r->body);
        const cJSON *metadata = cJSON_GetObjectItem(json, "metadata");
        if (metadata != NULL) {
            const cJSON *indices = cJSON_GetObjectItem(metadata, "indices");
            const cJSON *sist2 = cJSON_GetObjectItem(indices, "sist2");
            const cJSON *state = cJSON_GetObjectItem(sist2, "state");
            strcpy(status, state->valuestring);
        }
    }
    free_response(r);
    cJSON_Delete(json);
    return status;
}
