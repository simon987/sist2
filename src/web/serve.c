#include "serve.h"

#include "src/sist.h"
#include "src/io/store.h"
#include "static_generated.c"
#include "src/index/elastic.h"
#include "src/index/web.h"

#include <src/ctx.h>

#include <mongoose.h>


static int has_prefix(const struct mg_str *str, const struct mg_str *prefix) {
    return str->len > prefix->len && memcmp(str->p, prefix->p, prefix->len) == 0;
}

static int is_equal(const struct mg_str *s1, const struct mg_str *s2) {
    return s1->len == s2->len && memcmp(s1->p, s2->p, s2->len) == 0;
}

static void send_response_line(struct mg_connection *nc, int status_code, int length, char *extra_headers) {
    mg_printf(
            nc,
            "HTTP/1.1 %d %s\r\n"
            "Server: sist2\r\n"
            "Content-Length: %d\r\n"
            "Connection: close\r\n"
            "%s\r\n\r\n",
            status_code, "OK",
            length,
            extra_headers
    );
}


index_t *get_index_by_id(const char *index_id) {
    for (int i = WebCtx.index_count; i >= 0; i--) {
        if (strcmp(index_id, WebCtx.indices[i].desc.uuid) == 0) {
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

void search_index(struct mg_connection *nc) {
    send_response_line(nc, 200, sizeof(search_html), "Content-Type: text/html");
    mg_send(nc, search_html, sizeof(search_html));
    nc->flags |= MG_F_SEND_AND_CLOSE;
}

void stats(struct mg_connection *nc) {
    send_response_line(nc, 200, sizeof(stats_html), "Content-Type: text/html");
    mg_send(nc, stats_html, sizeof(stats_html));
    nc->flags |= MG_F_SEND_AND_CLOSE;
}

void stats_files(struct mg_connection *nc, struct http_message *hm, struct mg_str *path) {

    if (path->len != UUID_STR_LEN + 4) {
        mg_http_send_error(nc, 404, NULL);
        nc->flags |= MG_F_SEND_AND_CLOSE;
        return;
    }

    char arg_uuid[UUID_STR_LEN];
    memcpy(arg_uuid, hm->uri.p + 3, UUID_STR_LEN);
    *(arg_uuid + UUID_STR_LEN - 1) = '\0';

    index_t *index = get_index_by_id(arg_uuid);
    if (index == NULL) {
        mg_http_send_error(nc, 404, NULL);
        nc->flags |= MG_F_SEND_AND_CLOSE;
        return;
    }

    const char *file;
    switch (atoi(hm->uri.p + 3 + UUID_STR_LEN)) {
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
            nc->flags |= MG_F_SEND_AND_CLOSE;
            return;
    }

    char disposition[8192];
    snprintf(disposition, sizeof(disposition), "Content-Disposition: inline; filename=\"%s\"", file);

    char full_path[PATH_MAX];
    strcpy(full_path, index->path);
    strcat(full_path, file);

    mg_http_serve_file(nc, hm, full_path, mg_mk_str("text/csv"), mg_mk_str(disposition));
    nc->flags |= MG_F_SEND_AND_CLOSE;
}

void javascript_lib(struct mg_connection *nc) {
    send_response_line(nc, 200, sizeof(bundle_js), "Content-Type: application/javascript");
    mg_send(nc, bundle_js, sizeof(bundle_js));
    nc->flags |= MG_F_SEND_AND_CLOSE;
}

void javascript_search(struct mg_connection *nc) {
    send_response_line(nc, 200, sizeof(search_js), "Content-Type: application/javascript");
    mg_send(nc, search_js, sizeof(search_js));
    nc->flags |= MG_F_SEND_AND_CLOSE;
}

int client_requested_dark_theme(struct http_message *hm) {
    struct mg_str *cookie_header = mg_get_http_header(hm, "cookie");
    if (cookie_header == NULL) {
        return FALSE;
    }

    char buf[4096];
    char *sist_cookie = buf;
    if (mg_http_parse_header2(cookie_header, "sist", &sist_cookie, sizeof(buf)) == 0) {
        return FALSE;
    }

    int ret = strcmp(sist_cookie, "dark") == 0;
    if (sist_cookie != buf) {
        free(sist_cookie);
    }

    return ret;
}

void style(struct mg_connection *nc, struct http_message *hm) {

    if (client_requested_dark_theme(hm)) {
        send_response_line(nc, 200, sizeof(bundle_dark_css), "Content-Type: text/css");
        mg_send(nc, bundle_dark_css, sizeof(bundle_dark_css));
    } else {
        send_response_line(nc, 200, sizeof(bundle_css), "Content-Type: text/css");
        mg_send(nc, bundle_css, sizeof(bundle_css));
    }

    nc->flags |= MG_F_SEND_AND_CLOSE;
}

void img_sprite_skin_flat(struct mg_connection *nc, struct http_message *hm) {
    if (client_requested_dark_theme(hm)) {
        send_response_line(nc, 200, sizeof(sprite_skin_flat_dark_png), "Content-Type: image/png");
        mg_send(nc, sprite_skin_flat_dark_png, sizeof(sprite_skin_flat_dark_png));
    } else {
        send_response_line(nc, 200, sizeof(sprite_skin_flat_png), "Content-Type: image/png");
        mg_send(nc, sprite_skin_flat_png, sizeof(sprite_skin_flat_png));
    }

    nc->flags |= MG_F_SEND_AND_CLOSE;
}

void thumbnail(struct mg_connection *nc, struct http_message *hm, struct mg_str *path) {

    if (path->len != UUID_STR_LEN * 2 + 2) {
        LOG_DEBUGF("serve.c", "Invalid thumbnail path: %.*s", (int) path->len, path->p)
        mg_http_send_error(nc, 404, NULL);
        nc->flags |= MG_F_SEND_AND_CLOSE;
        return;
    }

    char arg_uuid[UUID_STR_LEN];
    char arg_index[UUID_STR_LEN];

    memcpy(arg_index, hm->uri.p + 3, UUID_STR_LEN);
    *(arg_index + UUID_STR_LEN - 1) = '\0';
    memcpy(arg_uuid, hm->uri.p + 3 + UUID_STR_LEN, UUID_STR_LEN);
    *(arg_uuid + UUID_STR_LEN - 1) = '\0';

    uuid_t uuid;
    int ret = uuid_parse(arg_uuid, uuid);
    if (ret != 0) {
        LOG_DEBUGF("serve.c", "Invalid thumbnail UUID: %s", arg_uuid)
        mg_http_send_error(nc, 404, NULL);
        nc->flags |= MG_F_SEND_AND_CLOSE;
        return;
    }

    store_t *store = get_store(arg_index);
    if (store == NULL) {
        LOG_DEBUGF("serve.c", "Could not get store for index: %s", arg_index)
        mg_http_send_error(nc, 404, NULL);
        nc->flags |= MG_F_SEND_AND_CLOSE;
        return;
    }

    size_t data_len = 0;
    char *data = store_read(store, (char *) uuid, sizeof(uuid_t), &data_len);
    if (data_len != 0) {
        send_response_line(nc, 200, data_len, "Content-Type: image/jpeg");
        mg_send(nc, data, data_len);
        free(data);
    }
    nc->flags |= MG_F_SEND_AND_CLOSE;
}

void search(struct mg_connection *nc, struct http_message *hm) {

    if (hm->body.len == 0) {
        LOG_DEBUG("serve.c", "Client sent empty body, ignoring request")
        mg_http_send_error(nc, 500, NULL);
        nc->flags |= MG_F_SEND_AND_CLOSE;
        return;
    }

    char *body = malloc(hm->body.len + 1);
    memcpy(body, hm->body.p, hm->body.len);
    *(body + hm->body.len) = '\0';

    char url[4096];
    snprintf(url, 4096, "%s/%s/_search", WebCtx.es_url, WebCtx.es_index);

    nc->user_data = web_post_async(url, body);
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
    mg_http_send_redirect(
            nc, 308,
            (struct mg_str) MG_MK_STR_N(encoded.buf, encoded.cur),
            (struct mg_str) MG_NULL_STR
    );
    dyn_buffer_destroy(&encoded);
    nc->flags |= MG_F_SEND_AND_CLOSE;
}

void serve_file_from_disk(cJSON *json, index_t *idx, struct mg_connection *nc, struct http_message *hm) {

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
    snprintf(disposition, sizeof(disposition), "Content-Disposition: inline; filename=\"%s%s%s\"",
             name, strlen(ext) == 0 ? "" : ".", ext);

    mg_http_serve_file(nc, hm, full_path, mg_mk_str(mime), mg_mk_str(disposition));
}

void index_info(struct mg_connection *nc) {
    cJSON *json = cJSON_CreateObject();
    cJSON *arr = cJSON_AddArrayToObject(json, "indices");

    for (int i = 0; i < WebCtx.index_count; i++) {
        index_t *idx = &WebCtx.indices[i];

        cJSON *idx_json = cJSON_CreateObject();
        cJSON_AddStringToObject(idx_json, "name", idx->desc.name);
        cJSON_AddStringToObject(idx_json, "version", idx->desc.version);
        cJSON_AddStringToObject(idx_json, "id", idx->desc.uuid);
        cJSON_AddNumberToObject(idx_json, "timestamp", (double) idx->desc.timestamp);
        cJSON_AddItemToArray(arr, idx_json);
    }

    char *json_str = cJSON_PrintUnformatted(json);

    send_response_line(nc, 200, strlen(json_str), "Content-Type: application/json");
    mg_send(nc, json_str, strlen(json_str));
    free(json_str);
    cJSON_Delete(json);

    nc->flags |= MG_F_SEND_AND_CLOSE;
}


void document_info(struct mg_connection *nc, struct http_message *hm, struct mg_str *path) {

    if (path->len != UUID_STR_LEN + 2) {
        LOG_DEBUGF("serve.c", "Invalid document_info path: %.*s", (int) path->len, path->p)
        mg_http_send_error(nc, 404, NULL);
        nc->flags |= MG_F_SEND_AND_CLOSE;
        return;
    }

    char arg_uuid[UUID_STR_LEN];
    memcpy(arg_uuid, hm->uri.p + 3, UUID_STR_LEN);
    *(arg_uuid + UUID_STR_LEN - 1) = '\0';

    cJSON *doc = elastic_get_document(arg_uuid);
    cJSON *source = cJSON_GetObjectItem(doc, "_source");

    cJSON *index_id = cJSON_GetObjectItem(source, "index");
    if (index_id == NULL) {
        cJSON_Delete(doc);
        mg_http_send_error(nc, 404, NULL);
        nc->flags |= MG_F_SEND_AND_CLOSE;
        return;
    }

    index_t *idx = get_index_by_id(index_id->valuestring);
    if (idx == NULL) {
        cJSON_Delete(doc);
        mg_http_send_error(nc, 404, NULL);
        nc->flags |= MG_F_SEND_AND_CLOSE;
        return;
    }

    char *json_str = cJSON_PrintUnformatted(source);
    send_response_line(nc, 200, (int) strlen(json_str), "Content-Type: application/json");
    mg_send(nc, json_str, (int) strlen(json_str));
    free(json_str);
    cJSON_Delete(doc);

    nc->flags |= MG_F_SEND_AND_CLOSE;
}

void file(struct mg_connection *nc, struct http_message *hm, struct mg_str *path) {

    if (path->len != UUID_STR_LEN + 2) {
        LOG_DEBUGF("serve.c", "Invalid file path: %.*s", (int) path->len, path->p)
        mg_http_send_error(nc, 404, NULL);
        nc->flags |= MG_F_SEND_AND_CLOSE;
        return;
    }

    char arg_uuid[UUID_STR_LEN];
    memcpy(arg_uuid, hm->uri.p + 3, UUID_STR_LEN);
    *(arg_uuid + UUID_STR_LEN - 1) = '\0';

    const char *next = arg_uuid;
    cJSON *doc = NULL;
    cJSON *index_id = NULL;
    cJSON *source = NULL;

    while (true) {
        doc = elastic_get_document(next);
        source = cJSON_GetObjectItem(doc, "_source");
        index_id = cJSON_GetObjectItem(source, "index");
        if (index_id == NULL) {
            cJSON_Delete(doc);
            mg_http_send_error(nc, 404, NULL);
            nc->flags |= MG_F_SEND_AND_CLOSE;
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
        nc->flags |= MG_F_SEND_AND_CLOSE;
        mg_http_send_error(nc, 404, NULL);
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

    nc->flags |= MG_F_SEND_AND_CLOSE;
}

typedef struct {
    char *name;
    int delete;
    char *relpath;
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

    cJSON *arg_relpath = cJSON_GetObjectItem(json, "relpath");
    if (arg_relpath == NULL || !cJSON_IsString(arg_relpath)) {
        return NULL;
    }

    cJSON *arg_doc_id = cJSON_GetObjectItem(json, "doc_id");
    if (arg_doc_id == NULL || !cJSON_IsString(arg_doc_id)) {
        return NULL;
    }

    tag_req_t *req = malloc(sizeof(tag_req_t));
    req->delete = arg_delete->valueint;
    req->name = arg_name->valuestring;
    req->relpath = arg_relpath->valuestring;
    req->doc_id = arg_doc_id->valuestring;

    return req;
}

void tag(struct mg_connection *nc, struct http_message *hm, struct mg_str *path) {
    if (path->len != UUID_STR_LEN + 4) {
        LOG_DEBUGF("serve.c", "Invalid tag path: %.*s", (int) path->len, path->p)
        mg_http_send_error(nc, 404, NULL);
        nc->flags |= MG_F_SEND_AND_CLOSE;
        return;
    }

    char arg_index[UUID_STR_LEN];
    memcpy(arg_index, hm->uri.p + 5, UUID_STR_LEN);
    *(arg_index + UUID_STR_LEN - 1) = '\0';

    if (hm->body.len < 2 || hm->method.len != 4 || memcmp(&hm->method, "POST", 4) == 0) {
        LOG_DEBUG("serve.c", "Invalid tag request")
        mg_http_send_error(nc, 400, NULL);
        nc->flags |= MG_F_SEND_AND_CLOSE;
        return;
    }

    store_t *store = get_tag_store(arg_index);
    if (store == NULL) {
        LOG_DEBUGF("serve.c", "Could not get tag store for index: %s", arg_index)
        mg_http_send_error(nc, 404, NULL);
        nc->flags |= MG_F_SEND_AND_CLOSE;
        return;
    }

    char *body = malloc(hm->body.len + 1);
    memcpy(body, hm->body.p, hm->body.len);
    *(body + hm->body.len) = '\0';
    cJSON *json = cJSON_Parse(body);

    tag_req_t *arg_req = parse_tag_request(json);
    if (arg_req == NULL) {
        LOG_DEBUGF("serve.c", "Could not parse tag request", arg_index)
        cJSON_Delete(json);
        free(body);
        mg_http_send_error(nc, 400, NULL);
        nc->flags |= MG_F_SEND_AND_CLOSE;
        return;
    }

    cJSON *arr = NULL;

    size_t data_len = 0;
    const char *data = store_read(store, arg_req->relpath, strlen(arg_req->relpath), &data_len);
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
        nc->user_data = web_post_async(url, buf);

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
        nc->user_data = web_post_async(url, buf);
    }

    char *json_str = cJSON_PrintUnformatted(arr);
    store_write(store, arg_req->relpath, strlen(arg_req->relpath) + 1, json_str, strlen(json_str) + 1);

    free(arg_req);
    free(json_str);
    cJSON_Delete(json);
    cJSON_Delete(arr);
    free(body);
}

int validate_auth(struct mg_connection *nc, struct http_message *hm) {
    char user[256] = {0,};
    char pass[256] = {0,};

    int ret = mg_get_http_basic_auth(hm, user, sizeof(user), pass, sizeof(pass));
    if (ret == -1 || strcmp(user, WebCtx.auth_user) != 0 || strcmp(pass, WebCtx.auth_pass) != 0) {
        mg_printf(nc, "HTTP/1.1 401 Unauthorized\r\n"
                      "WWW-Authenticate: Basic realm=\"sist2\"\r\n"
                      "Content-Length: 0\r\n\r\n");
        nc->flags |= MG_F_SEND_AND_CLOSE;
        return FALSE;
    }
    return TRUE;
}

static void ev_router(struct mg_connection *nc, int ev, void *p) {
    struct mg_str scheme;
    struct mg_str user_info;
    struct mg_str host;
    unsigned int port;
    struct mg_str path;
    struct mg_str query;
    struct mg_str fragment;

    if (ev == MG_EV_HTTP_REQUEST) {
        struct http_message *hm = (struct http_message *) p;

        if (mg_parse_uri(hm->uri, &scheme, &user_info, &host, &port, &path, &query, &fragment) != 0) {
            mg_http_send_error(nc, 400, NULL);
            nc->flags |= MG_F_SEND_AND_CLOSE;
            return;
        }


        if (WebCtx.auth_enabled == TRUE) {
            if (!validate_auth(nc, hm)) {
                return;
            }
        }

        if (is_equal(&path, &((struct mg_str) MG_MK_STR("/")))) {
            search_index(nc);
        } else if (is_equal(&path, &((struct mg_str) MG_MK_STR("/css")))) {
            style(nc, hm);
        } else if (is_equal(&path, &((struct mg_str) MG_MK_STR("/stats")))) {
            stats(nc);
        } else if (is_equal(&path, &((struct mg_str) MG_MK_STR("/jslib")))) {
            javascript_lib(nc);
        } else if (is_equal(&path, &((struct mg_str) MG_MK_STR("/jssearch")))) {
            javascript_search(nc);
        } else if (is_equal(&path, &((struct mg_str) MG_MK_STR("/img/sprite-skin-flat.png")))) {
            img_sprite_skin_flat(nc, hm);
        } else if (is_equal(&path, &((struct mg_str) MG_MK_STR("/es")))) {
            search(nc, hm);
        } else if (is_equal(&path, &((struct mg_str) MG_MK_STR("/i")))) {
            index_info(nc);
        } else if (is_equal(&path, &((struct mg_str) MG_MK_STR("/status")))) {
            status(nc);
        } else if (has_prefix(&path, &((struct mg_str) MG_MK_STR("/f/")))) {
            file(nc, hm, &path);
        } else if (has_prefix(&path, &((struct mg_str) MG_MK_STR("/t/")))) {
            thumbnail(nc, hm, &path);
        } else if (has_prefix(&path, &((struct mg_str) MG_MK_STR("/s/")))) {
            stats_files(nc, hm, &path);
        } else if (has_prefix(&path, &((struct mg_str) MG_MK_STR("/tag/")))) {
            if (WebCtx.tag_auth_enabled == TRUE) {
                if (!validate_auth(nc, hm)) {
                    return;
                }
            }
            tag(nc, hm, &path);
        } else if (has_prefix(&path, &((struct mg_str) MG_MK_STR("/d/")))) {
            document_info(nc, hm, &path);
        } else {
            mg_http_send_error(nc, 404, NULL);
            nc->flags |= MG_F_SEND_AND_CLOSE;
        }

    } else if (ev == MG_EV_POLL) {
        if (nc->user_data != NULL) {
            //Waiting for ES reply
            subreq_ctx_t *ctx = (subreq_ctx_t *) nc->user_data;
            web_post_async_poll(ctx);

            if (ctx->done == TRUE) {

                response_t *r = ctx->response;

                if (r->status_code == 200) {
                    send_response_line(nc, 200, r->size, "Content-Type: application/json");
                    mg_send(nc, r->body, r->size);
                } else if (r->status_code == 0) {
                    sist_log("serve.c", SIST_ERROR, "Could not connect to elasticsearch!");
                } else {
                    sist_logf("serve.c", SIST_WARNING, "ElasticSearch error during query (%d)", r->status_code);
                    if (r->size != 0) {
                        char *tmp = malloc(r->size + 1);
                        memcpy(tmp, r->body, r->size);
                        *(tmp + r->size) = '\0';
                        cJSON *json = cJSON_Parse(tmp);
                        char *json_str = cJSON_Print(json);
                        sist_log("serve.c", SIST_WARNING, json_str);
                        free(json_str);
                        free(tmp);
                    }
                    mg_http_send_error(nc, 500, NULL);
                }

                free_response(r);
                free(ctx->data);
                free(ctx);
                nc->flags |= MG_F_SEND_AND_CLOSE;
                nc->user_data = NULL;
            }
        }
    }
}

void serve(const char *listen_address) {

    printf("Starting web server @ http://%s\n", listen_address);

    struct mg_mgr mgr;
    mg_mgr_init(&mgr, NULL);

    struct mg_connection *nc = mg_bind(&mgr, listen_address, ev_router);
    if (nc == NULL) {
        LOG_FATALF("serve.c", "Couldn't bind web server on address %s", listen_address)
    }
    mg_set_protocol_http_websocket(nc);

    for (;;) {
        mg_mgr_poll(&mgr, 10);
    }
}
