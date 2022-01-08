#include "serve.h"

#include "src/sist.h"
#include "src/io/store.h"
#include "static_generated.c"
#include "src/index/elastic.h"
#include "src/index/web.h"

#include <src/ctx.h>


static void send_response_line(struct mg_connection *nc, int status_code, size_t length, char *extra_headers) {
    mg_printf(
            nc,
            "HTTP/1.1 %d %s\r\n"
            "Server: sist2/" VERSION "\r\n"
            "Content-Length: %d\r\n"
            "%s\r\n\r\n",
            status_code, "OK",
            length,
            extra_headers
    );
}


index_t *get_index_by_id(const char *index_id) {
    for (int i = WebCtx.index_count; i >= 0; i--) {
        if (strncmp(index_id, WebCtx.indices[i].desc.id, MD5_STR_LENGTH) == 0) {
            return &WebCtx.indices[i];
        }
    }
    return NULL;
}

store_t *get_store(const char *index_id) {
    index_t *idx = get_index_by_id(index_id);
    if (idx != NULL) {
        return idx->store;
    }
    return NULL;
}

store_t *get_tag_store(const char *index_id) {
    index_t *idx = get_index_by_id(index_id);
    if (idx != NULL) {
        return idx->tag_store;
    }
    return NULL;
}

void search_index(struct mg_connection *nc, struct mg_http_message *hm) {
    if (WebCtx.dev) {
        mg_http_serve_file(nc, hm, "sist2-vue/dist/index.html", "text/html", NULL);
    } else {
        send_response_line(nc, 200, sizeof(index_html), "Content-Type: text/html");
        mg_send(nc, index_html, sizeof(index_html));
    }
}

void stats_files(struct mg_connection *nc, struct mg_http_message *hm) {

    if (hm->uri.len != MD5_STR_LENGTH + 4) {
        mg_http_reply(nc, 404, "", "");
        return;
    }

    char arg_md5[MD5_STR_LENGTH];
    memcpy(arg_md5, hm->uri.ptr + 3, MD5_STR_LENGTH);
    *(arg_md5 + MD5_STR_LENGTH - 1) = '\0';

    index_t *index = get_index_by_id(arg_md5);
    if (index == NULL) {
        mg_http_reply(nc, 404, "", "");
        return;
    }

    const char *file;
    switch (atoi(hm->uri.ptr + 3 + MD5_STR_LENGTH)) {
        case 1:
            file = "treemap.csv";
            break;
        case 2:
            file = "mime_agg.csv";
            break;
        case 3:
            file = "size_agg.csv";
            break;
        case 4:
            file = "date_agg.csv";
            break;
        default:
            return;
    }

    char disposition[8192];
    snprintf(disposition, sizeof(disposition),
             "Content-Disposition: inline; filename=\"%s\"\r\nCache-Control: max-age=31536000\r\n", file);

    char full_path[PATH_MAX];
    strcpy(full_path, index->path);
    strcat(full_path, file);

    mg_http_serve_file(nc, hm, full_path, "text/csv", disposition);
}

void javascript(struct mg_connection *nc, struct mg_http_message *hm) {
    if (WebCtx.dev) {
        mg_http_serve_file(nc, hm, "sist2-vue/dist/js/index.js", "application/javascript", NULL);
    } else {
        send_response_line(nc, 200, sizeof(index_js), "Content-Type: application/javascript");
        mg_send(nc, index_js, sizeof(index_js));
    }
}

void javascript_vendor(struct mg_connection *nc, struct mg_http_message *hm) {
    if (WebCtx.dev) {
        mg_http_serve_file(nc, hm, "sist2-vue/dist/js/chunk-vendors.js", "application/javascript", NULL);
    } else {
        send_response_line(nc, 200, sizeof(chunk_vendors_js), "Content-Type: application/javascript");
        mg_send(nc, chunk_vendors_js, sizeof(chunk_vendors_js));
    }
}

void favicon(struct mg_connection *nc, struct mg_http_message *hm) {
    send_response_line(nc, 200, sizeof(favicon_ico), "Content-Type: image/x-icon");
    mg_send(nc, favicon_ico, sizeof(favicon_ico));
}

void style(struct mg_connection *nc, struct mg_http_message *hm) {
    send_response_line(nc, 200, sizeof(index_css), "Content-Type: text/css");
    mg_send(nc, index_css, sizeof(index_css));
}

void style_vendor(struct mg_connection *nc, struct mg_http_message *hm) {
    send_response_line(nc, 200, sizeof(chunk_vendors_css), "Content-Type: text/css");
    mg_send(nc, chunk_vendors_css, sizeof(chunk_vendors_css));
}

void thumbnail(struct mg_connection *nc, struct mg_http_message *hm) {

    if (hm->uri.len != 68) {
        LOG_DEBUGF("serve.c", "Invalid thumbnail path: %.*s", (int) hm->uri.len, hm->uri.ptr)
        mg_http_reply(nc, 404, "", "Not found");
        return;
    }

    char arg_file_md5[MD5_STR_LENGTH];
    char arg_index[MD5_STR_LENGTH];

    memcpy(arg_index, hm->uri.ptr + 3, MD5_STR_LENGTH);
    *(arg_index + MD5_STR_LENGTH - 1) = '\0';
    memcpy(arg_file_md5, hm->uri.ptr + 3 + MD5_STR_LENGTH, MD5_STR_LENGTH);
    *(arg_file_md5 + MD5_STR_LENGTH - 1) = '\0';

    unsigned char md5_buf[MD5_DIGEST_LENGTH];
    hex2buf(arg_file_md5, MD5_STR_LENGTH - 1, md5_buf);

    store_t *store = get_store(arg_index);
    if (store == NULL) {
        LOG_DEBUGF("serve.c", "Could not get store for index: %s", arg_index)
        mg_http_reply(nc, 404, "", "Not found");
        return;
    }

    size_t data_len = 0;
    char *data = store_read(store, (char *) md5_buf, sizeof(md5_buf), &data_len);
    if (data_len != 0) {
        send_response_line(
                nc, 200, data_len,
                "Content-Type: image/jpeg\r\n"
                "Cache-Control: max-age=31536000"
        );
        mg_send(nc, data, data_len);
        free(data);
    } else {
        mg_http_reply(nc, 404, "Content-Type: text/plain;charset=utf-8\r\n", "Not found");
        return;
    }
}

void search(struct mg_connection *nc, struct mg_http_message *hm) {

    if (hm->body.len == 0) {
        LOG_DEBUG("serve.c", "Client sent empty body, ignoring request")
        mg_http_reply(nc, 500, "", "Invalid request");
        return;
    }

    char *body = malloc(hm->body.len + 1);
    memcpy(body, hm->body.ptr, hm->body.len);
    *(body + hm->body.len) = '\0';

    char url[4096];
    snprintf(url, 4096, "%s/%s/_search", WebCtx.es_url, WebCtx.es_index);

    nc->fn_data = web_post_async(url, body);
}

void serve_file_from_url(cJSON *json, index_t *idx, struct mg_connection *nc) {

    const char *path = cJSON_GetObjectItem(json, "path")->valuestring;
    const char *name = cJSON_GetObjectItem(json, "name")->valuestring;

    char name_unescaped[PATH_MAX * 3];
    str_unescape(name_unescaped, name);

    char path_unescaped[PATH_MAX * 3];
    str_unescape(path_unescaped, path);

    const char *ext = cJSON_GetObjectItem(json, "extension")->valuestring;

    char url[8192];
    snprintf(url, sizeof(url),
             "%s%s/%s%s%s",
             idx->desc.rewrite_url, path_unescaped, name_unescaped, strlen(ext) == 0 ? "" : ".", ext);

    dyn_buffer_t encoded = url_escape(url);
    dyn_buffer_write_char(&encoded, '\0');

    char location_header[8192];
    snprintf(location_header, sizeof(location_header), "Location: %s\r\n", encoded.buf);

    mg_http_reply(nc, 308, location_header, "");
    dyn_buffer_destroy(&encoded);
}

void serve_file_from_disk(cJSON *json, index_t *idx, struct mg_connection *nc, struct mg_http_message *hm) {

    const char *path = cJSON_GetObjectItem(json, "path")->valuestring;
    const char *name = cJSON_GetObjectItem(json, "name")->valuestring;
    const char *ext = cJSON_GetObjectItem(json, "extension")->valuestring;
    const char *mime = cJSON_GetObjectItem(json, "mime")->valuestring;

    char name_unescaped[PATH_MAX * 3];
    str_unescape(name_unescaped, name);

    char path_unescaped[PATH_MAX * 3];
    str_unescape(path_unescaped, path);

    char full_path[PATH_MAX];
    snprintf(full_path, PATH_MAX, "%s%s%s%s%s%s",
             idx->desc.root, path_unescaped, strlen(path_unescaped) == 0 ? "" : "/",
             name_unescaped, strlen(ext) == 0 ? "" : ".", ext);

    LOG_DEBUGF("serve.c", "Serving file from disk: %s", full_path)

    char disposition[8192];
    snprintf(disposition, sizeof(disposition),
             "Content-Disposition: inline; filename=\"%s%s%s\"\r\nAccept-Ranges: bytes\r\n",
             name, strlen(ext) == 0 ? "" : ".", ext);

    mg_http_serve_file(nc, hm, full_path, mime, disposition);
}

void cache_es_version() {
    static int is_cached = FALSE;

    if (is_cached == TRUE) {
        return;
    }

    es_version_t *es_version = elastic_get_version(WebCtx.es_url);
    if (es_version != NULL) {
        WebCtx.es_version = es_version;
        is_cached = TRUE;
    }
}

void index_info(struct mg_connection *nc) {

    cache_es_version();

    cJSON *json = cJSON_CreateObject();
    cJSON *arr = cJSON_AddArrayToObject(json, "indices");

    cJSON_AddStringToObject(json, "esIndex", WebCtx.es_index);
    cJSON_AddStringToObject(json, "version", Version);
    cJSON_AddStringToObject(json, "esVersion", format_es_version(WebCtx.es_version));
    cJSON_AddBoolToObject(json, "esVersionSupported", IS_SUPPORTED_ES_VERSION(WebCtx.es_version));
    cJSON_AddBoolToObject(json, "esVersionLegacy", USE_LEGACY_ES_SETTINGS(WebCtx.es_version));
    cJSON_AddStringToObject(json, "platform", QUOTE(SIST_PLATFORM));
    cJSON_AddStringToObject(json, "sist2Hash", Sist2CommitHash);
    cJSON_AddStringToObject(json, "lang", WebCtx.lang);
    cJSON_AddBoolToObject(json, "dev", WebCtx.dev);
#ifdef SIST_DEBUG
    cJSON_AddBoolToObject(json, "debug", TRUE);
#else
    cJSON_AddBoolToObject(json, "debug", FALSE);
#endif
    cJSON_AddStringToObject(json, "tagline", WebCtx.tagline);

    for (int i = 0; i < WebCtx.index_count; i++) {
        index_t *idx = &WebCtx.indices[i];

        cJSON *idx_json = cJSON_CreateObject();
        cJSON_AddStringToObject(idx_json, "name", idx->desc.name);
        cJSON_AddStringToObject(idx_json, "version", idx->desc.version);
        cJSON_AddStringToObject(idx_json, "id", idx->desc.id);
        cJSON_AddStringToObject(idx_json, "rewriteUrl", idx->desc.rewrite_url);
        cJSON_AddNumberToObject(idx_json, "timestamp", (double) idx->desc.timestamp);
        cJSON_AddItemToArray(arr, idx_json);
    }

    char *json_str = cJSON_PrintUnformatted(json);

    send_response_line(nc, 200, strlen(json_str), "Content-Type: application/json");
    mg_send(nc, json_str, strlen(json_str));
    free(json_str);
    cJSON_Delete(json);
}


void document_info(struct mg_connection *nc, struct mg_http_message *hm) {

    if (hm->uri.len != MD5_STR_LENGTH + 2) {
        LOG_DEBUGF("serve.c", "Invalid document_info path: %.*s", (int) hm->uri.len, hm->uri.ptr)
        mg_http_reply(nc, 404, "", "Not found");
        return;
    }

    char arg_md5[MD5_STR_LENGTH];
    memcpy(arg_md5, hm->uri.ptr + 3, MD5_STR_LENGTH);
    *(arg_md5 + MD5_STR_LENGTH - 1) = '\0';

    cJSON *doc = elastic_get_document(arg_md5);
    cJSON *source = cJSON_GetObjectItem(doc, "_source");

    cJSON *index_id = cJSON_GetObjectItem(source, "index");
    if (index_id == NULL) {
        cJSON_Delete(doc);
        mg_http_reply(nc, 404, "", "Not found");
        return;
    }

    index_t *idx = get_index_by_id(index_id->valuestring);
    if (idx == NULL) {
        cJSON_Delete(doc);
        mg_http_reply(nc, 404, "", "Not found");
        return;
    }

    char *json_str = cJSON_PrintUnformatted(source);
    send_response_line(nc, 200, (int) strlen(json_str), "Content-Type: application/json");
    mg_send(nc, json_str, (int) strlen(json_str));
    free(json_str);
    cJSON_Delete(doc);
}

void file(struct mg_connection *nc, struct mg_http_message *hm) {

    if (hm->uri.len != MD5_STR_LENGTH + 2) {
        LOG_DEBUGF("serve.c", "Invalid file path: %.*s", (int) hm->uri.len, hm->uri.ptr)
        mg_http_reply(nc, 404, "", "Not found");
        return;
    }

    char arg_md5[MD5_STR_LENGTH];
    memcpy(arg_md5, hm->uri.ptr + 3, MD5_STR_LENGTH);
    *(arg_md5 + MD5_STR_LENGTH - 1) = '\0';

    const char *next = arg_md5;
    cJSON *doc = NULL;
    cJSON *index_id = NULL;
    cJSON *source = NULL;

    while (true) {
        doc = elastic_get_document(next);
        source = cJSON_GetObjectItem(doc, "_source");
        index_id = cJSON_GetObjectItem(source, "index");
        if (index_id == NULL) {
            cJSON_Delete(doc);
            mg_http_reply(nc, 404, "", "Not found");
            return;
        }
        cJSON *parent = cJSON_GetObjectItem(source, "parent");
        if (parent == NULL) {
            break;
        }
        next = parent->valuestring;
    }

    index_t *idx = get_index_by_id(index_id->valuestring);

    if (idx == NULL) {
        cJSON_Delete(doc);
        mg_http_reply(nc, 404, "", "Not found");
        return;
    }

    if (strlen(idx->desc.rewrite_url) == 0) {
        serve_file_from_disk(source, idx, nc, hm);
    } else {
        serve_file_from_url(source, idx, nc);
    }
    cJSON_Delete(doc);
}

void status(struct mg_connection *nc) {
    char *status = elastic_get_status();
    if (strcmp(status, "open") == 0) {
        send_response_line(nc, 204, 0, "Content-Type: application/json");
    } else {
        send_response_line(nc, 500, 0, "Content-Type: application/json");
    }

    free(status);
}

typedef struct {
    char *name;
    int delete;
    char *path_md5_str;
    char *doc_id;
} tag_req_t;

tag_req_t *parse_tag_request(cJSON *json) {

    if (!cJSON_IsObject(json)) {
        return NULL;
    }

    cJSON *arg_name = cJSON_GetObjectItem(json, "name");
    if (arg_name == NULL || !cJSON_IsString(arg_name)) {
        return NULL;
    }

    cJSON *arg_delete = cJSON_GetObjectItem(json, "delete");
    if (arg_delete == NULL || !cJSON_IsBool(arg_delete)) {
        return NULL;
    }

    cJSON *arg_path_md5 = cJSON_GetObjectItem(json, "path_md5");
    if (arg_path_md5 == NULL || !cJSON_IsString(arg_path_md5) ||
        strlen(arg_path_md5->valuestring) != MD5_STR_LENGTH - 1) {
        return NULL;
    }

    cJSON *arg_doc_id = cJSON_GetObjectItem(json, "doc_id");
    if (arg_doc_id == NULL || !cJSON_IsString(arg_doc_id)) {
        return NULL;
    }

    tag_req_t *req = malloc(sizeof(tag_req_t));
    req->delete = arg_delete->valueint;
    req->name = arg_name->valuestring;
    req->path_md5_str = arg_path_md5->valuestring;
    req->doc_id = arg_doc_id->valuestring;

    return req;
}

void tag(struct mg_connection *nc, struct mg_http_message *hm) {
    if (hm->uri.len != MD5_STR_LENGTH + 4) {
        LOG_DEBUGF("serve.c", "Invalid tag path: %.*s", (int) hm->uri.len, hm->uri.ptr)
        mg_http_reply(nc, 404, "", "Not found");
        return;
    }

    char arg_index[MD5_STR_LENGTH];
    memcpy(arg_index, hm->uri.ptr + 5, MD5_STR_LENGTH);
    *(arg_index + MD5_STR_LENGTH - 1) = '\0';

    if (hm->body.len < 2 || hm->method.len != 4 || memcmp(&hm->method, "POST", 4) == 0) {
        LOG_DEBUG("serve.c", "Invalid tag request")
        mg_http_reply(nc, 404, "", "Not found");
        return;
    }

    store_t *store = get_tag_store(arg_index);
    if (store == NULL) {
        LOG_DEBUGF("serve.c", "Could not get tag store for index: %s", arg_index)
        mg_http_reply(nc, 404, "", "Not found");
        return;
    }

    char *body = malloc(hm->body.len + 1);
    memcpy(body, hm->body.ptr, hm->body.len);
    *(body + hm->body.len) = '\0';
    cJSON *json = cJSON_Parse(body);

    tag_req_t *arg_req = parse_tag_request(json);
    if (arg_req == NULL) {
        LOG_DEBUGF("serve.c", "Could not parse tag request", arg_index)
        cJSON_Delete(json);
        free(body);
        mg_http_reply(nc, 400, "", "Invalid request");
        return;
    }

    cJSON *arr = NULL;

    size_t data_len = 0;
    const char *data = store_read(store, arg_req->path_md5_str, MD5_STR_LENGTH, &data_len);
    if (data_len == 0) {
        arr = cJSON_CreateArray();
    } else {
        arr = cJSON_Parse(data);
    }

    if (arg_req->delete) {

        if (data_len > 0) {
            cJSON *element = NULL;
            int i = 0;
            cJSON_ArrayForEach(element, arr) {
                if (strcmp(element->valuestring, arg_req->name) == 0) {
                    cJSON_DeleteItemFromArray(arr, i);
                    break;
                }
                i++;
            }
        }

        char *buf = malloc(sizeof(char) * 8192);
        snprintf(buf, 8192,
                 "{"
                 "    \"script\" : {"
                 "        \"source\": \"if (ctx._source.tag.contains(params.tag)) { ctx._source.tag.remove(ctx._source.tag.indexOf(params.tag)) }\","
                 "        \"lang\": \"painless\","
                 "        \"params\" : {"
                 "            \"tag\" : \"%s\""
                 "        }"
                 "    }"
                 "}", arg_req->name
        );

        char url[4096];
        snprintf(url, sizeof(url), "%s/%s/_update/%s", WebCtx.es_url, WebCtx.es_index, arg_req->doc_id);
        nc->fn_data = web_post_async(url, buf);

    } else {
        cJSON_AddItemToArray(arr, cJSON_CreateString(arg_req->name));

        char *buf = malloc(sizeof(char) * 8192);
        snprintf(buf, 8192,
                 "{"
                 "    \"script\" : {"
                 "        \"source\": \"if(ctx._source.tag == null) {ctx._source.tag = new ArrayList()} ctx._source.tag.add(params.tag)\","
                 "        \"lang\": \"painless\","
                 "        \"params\" : {"
                 "            \"tag\" : \"%s\""
                 "        }"
                 "    }"
                 "}", arg_req->name
        );

        char url[4096];
        snprintf(url, sizeof(url), "%s/%s/_update/%s", WebCtx.es_url, WebCtx.es_index, arg_req->doc_id);
        nc->fn_data = web_post_async(url, buf);
    }

    char *json_str = cJSON_PrintUnformatted(arr);
    store_write(store, arg_req->path_md5_str, MD5_STR_LENGTH, json_str, strlen(json_str) + 1);
    store_flush(store);

    free(arg_req);
    free(json_str);
    cJSON_Delete(json);
    cJSON_Delete(arr);
    free(body);
}

int validate_auth(struct mg_connection *nc, struct mg_http_message *hm) {
    char user[256] = {0,};
    char pass[256] = {0,};

    mg_http_creds(hm, user, sizeof(user), pass, sizeof(pass));
    if (strcmp(user, WebCtx.auth_user) != 0 || strcmp(pass, WebCtx.auth_pass) != 0) {
        mg_http_reply(nc, 401, "WWW-Authenticate: Basic realm=\"sist2\"\r\n", "");
        return FALSE;
    }
    return TRUE;
}

static void ev_router(struct mg_connection *nc, int ev, void *ev_data, UNUSED(void *fn_data)) {

    if (ev == MG_EV_HTTP_MSG) {
        struct mg_http_message *hm = (struct mg_http_message *) ev_data;

        if (WebCtx.auth_enabled == TRUE) {
            if (!validate_auth(nc, hm)) {
                return;
            }
        }

        if (mg_http_match_uri(hm, "/")) {
            search_index(nc, hm);
        } else if (mg_http_match_uri(hm, "/favicon.ico")) {
            favicon(nc, hm);
        } else if (mg_http_match_uri(hm, "/css/index.css")) {
            style(nc, hm);
        } else if (mg_http_match_uri(hm, "/css/chunk-vendors.css")) {
            style_vendor(nc, hm);
        } else if (mg_http_match_uri(hm, "/js/index.js")) {
            javascript(nc, hm);
        } else if (mg_http_match_uri(hm, "/js/chunk-vendors.js")) {
            javascript_vendor(nc, hm);
        } else if (mg_http_match_uri(hm, "/es")) {
            search(nc, hm);
        } else if (mg_http_match_uri(hm, "/i")) {
            index_info(nc);
        } else if (mg_http_match_uri(hm, "/status")) {
            status(nc);
        } else if (mg_http_match_uri(hm, "/f/*")) {
            file(nc, hm);
        } else if (mg_http_match_uri(hm, "/t/*/*")) {
            thumbnail(nc, hm);
        } else if (mg_http_match_uri(hm, "/s/*/*")) {
            stats_files(nc, hm);
        } else if (mg_http_match_uri(hm, "/tag/*")) {
            if (WebCtx.tag_auth_enabled == TRUE && !validate_auth(nc, hm)) {
                return;
            }
            tag(nc, hm);
        } else if (mg_http_match_uri(hm, "/d/*")) {
            document_info(nc, hm);
        } else {
            mg_http_reply(nc, 404, "", "Page not found");
        }

    } else if (ev == MG_EV_POLL) {
        if (nc->fn_data != NULL) {
            //Waiting for ES reply
            subreq_ctx_t *ctx = (subreq_ctx_t *) nc->fn_data;
            web_post_async_poll(ctx);

            if (ctx->done == TRUE) {
                response_t *r = ctx->response;

                if (r->status_code == 200) {
                    send_response_line(nc, 200, r->size, "Content-Type: application/json");
                    mg_send(nc, r->body, r->size);
                } else if (r->status_code == 0) {
                    sist_log("serve.c", LOG_SIST_ERROR, "Could not connect to elasticsearch!");
                } else {
                    sist_logf("serve.c", LOG_SIST_WARNING, "ElasticSearch error during query (%d)", r->status_code);
                    if (r->size != 0) {
                        char *tmp = malloc(r->size + 1);
                        memcpy(tmp, r->body, r->size);
                        *(tmp + r->size) = '\0';
                        cJSON *json = cJSON_Parse(tmp);
                        char *json_str = cJSON_Print(json);
                        sist_log("serve.c", LOG_SIST_WARNING, json_str);
                        free(json_str);
                        free(tmp);
                    }

                    mg_http_reply(nc, 500, "", "");
                }

                free_response(r);
                free(ctx->data);
                free(ctx);
                nc->fn_data = NULL;
            }
        }
    }
}

void serve(const char *listen_address) {

    printf("Starting web server @ http://%s\n", listen_address);

    struct mg_mgr mgr;
    mg_mgr_init(&mgr);

    int ok = 1;

    struct mg_connection *nc = mg_http_listen(&mgr, listen_address, ev_router, NULL);
    if (nc == NULL) {
        LOG_FATALF("serve.c", "Couldn't bind web server on address %s", listen_address)
    }

    while (ok) {
        mg_mgr_poll(&mgr, 10);
    }
    mg_mgr_free(&mgr);
    LOG_INFO("serve.c", "Finished web event loop")
}
