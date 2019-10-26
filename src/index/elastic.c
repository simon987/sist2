#include "elastic.h"
#include "src/ctx.h"

#include <stdlib.h>
#include "web.h"
#include <stdio.h>
#include <string.h>
#include <cJSON/cJSON.h>
#include <src/ctx.h>

#include "static_generated.c"

#define BULK_INDEX_SIZE 100

typedef struct es_indexer {
    int queued;
    char *es_url;
    es_bulk_line_t *line_head;
    es_bulk_line_t *line_tail;
} es_indexer_t;


static es_indexer_t *Indexer;

void print_json(cJSON *document, const char uuid_str[UUID_STR_LEN]) {

    cJSON *line = cJSON_CreateObject();

    cJSON_AddStringToObject(line, "_id", uuid_str);
    cJSON_AddStringToObject(line, "_index", "sist2");
    cJSON_AddStringToObject(line, "_type", "_doc");
    cJSON_AddItemToObject(line, "_source", document);

    char *json = cJSON_PrintUnformatted(line);

    printf("%s\n", json);

    cJSON_free(line);
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
    elastic_index_line(bulk_line);
}

void elastic_flush() {

    if (Indexer == NULL) {
        Indexer = create_indexer(IndexCtx.es_url);
    }

    es_bulk_line_t *line = Indexer->line_head;

    int count = 0;

    size_t buf_size = 0;
    size_t buf_cur = 0;
    char *buf = malloc(1);

    while (line != NULL) {
        char action_str[512];
        snprintf(action_str, 512, "{\"index\":{\"_id\": \"%s\", \"_type\":\"_doc\"}}", line->uuid_str);
        size_t action_str_len = strlen(action_str);

        size_t line_len = strlen(line->line);
        buf = realloc(buf, buf_size + line_len + action_str_len);
        buf_size += line_len + action_str_len;

        memcpy(buf + buf_cur, action_str, action_str_len);
        buf_cur += action_str_len;
        memcpy(buf + buf_cur, line->line, line_len);
        buf_cur += line_len;

        es_bulk_line_t *tmp = line;
        line = line->next;
        free(tmp);
        count++;
    }
    buf = realloc(buf, buf_size + 1);
    *(buf+buf_cur) = '\0';

    Indexer->line_head = NULL;
    Indexer->line_tail = NULL;
    Indexer->queued = 0;

    char bulk_url[4096];
    snprintf(bulk_url, 4096, "%s/sist2/_bulk", Indexer->es_url);
    response_t *r = web_post(bulk_url, buf, "Content-Type: application/x-ndjson");
    printf("Indexed %3d documents (%zukB) <%d>\n", count, buf_cur / 1024, r->status_code);
    free_response(r);
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

    if (Indexer->queued >= BULK_INDEX_SIZE) {
        elastic_flush();
    }
}

es_indexer_t *create_indexer(const char *url) {

    size_t url_len = strlen(url);
    char *es_url = malloc(url_len);
    strcpy(es_url, url);

    es_indexer_t *indexer = malloc(sizeof(es_indexer_t));

    indexer->es_url = es_url;
    indexer->queued = 0;
    indexer->line_head = NULL;
    indexer->line_tail = NULL;

    return indexer;
}

void destroy_indexer() {

    char url[4096];

    snprintf(url, sizeof(url), "%s/sist2/_refresh", IndexCtx.es_url);
    response_t *r = web_post(url, "", NULL);
    printf("Refresh index <%d>\n", r->status_code);
    free_response(r);

    snprintf(url, sizeof(url), "%s/sist2/_forcemerge", IndexCtx.es_url);
    r = web_post(url, "", NULL);
    printf("Merge index <%d>\n", r->status_code);
    free_response(r);

    if (Indexer != NULL) {
        free(Indexer->es_url);
        free(Indexer);
    }
}

void elastic_init(int force_reset) {

    // Check if index exists
    char url[4096];
    snprintf(url, 4096, "%s/sist2", IndexCtx.es_url);
    response_t *r = web_get(url);
    int index_exists = r->status_code == 200;
    free_response(r);

    if (!index_exists || force_reset) {
        r = web_delete(url);
        printf("Delete index <%d>\n", r->status_code);
        free_response(r);

        snprintf(url, 4096, "%s/sist2", IndexCtx.es_url);
        r = web_put(url, "", NULL);
        printf("Create index <%d>\n", r->status_code);
        free_response(r);

        snprintf(url, 4096, "%s/sist2/_close", IndexCtx.es_url);
        r = web_post(url, "", NULL);
        printf("Close index <%d>\n", r->status_code);
        free_response(r);

        snprintf(url, 4096, "%s/sist2/_settings", IndexCtx.es_url);
        r = web_put(url, settings_json, "Content-Type: application/json");
        printf("Update settings <%d>\n", r->status_code);
        free_response(r);

        snprintf(url, 4096, "%s/sist2/_mappings/_doc", IndexCtx.es_url);
        r = web_put(url, mappings_json, "Content-Type: application/json");
        printf("Update mappings <%d>\n", r->status_code);
        free_response(r);

        snprintf(url, 4096, "%s/sist2/_open", IndexCtx.es_url);
        r = web_post(url, "", NULL);
        printf("Open index <%d>\n", r->status_code);
        free_response(r);
    }
}

cJSON *elastic_get_document(const char *uuid_str) {
    char url[4096];
    snprintf(url, 4096, "%s/sist2/_source/%s", WebCtx.es_url, uuid_str);

    response_t *r = web_get(url);
    return cJSON_Parse(r->body);
}
