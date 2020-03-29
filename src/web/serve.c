#include "serve.h"

#include "src/sist.h"
#include "src/io/store.h"
#include "static_generated.c"
#include "src/index/elastic.h"
#include "src/index/web.h"
#include "src/web/auth_basic.h"

#include <src/ctx.h>

#include <onion/onion.h>
#include <onion/handler.h>
#include <onion/block.h>
#include <onion/shortcuts.h>

#include <mongoose.h>

#define CHUNK_SIZE 1024 * 1024 * 10


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

void search_index(struct mg_connection *nc) {
    send_response_line(nc, 200, sizeof(search_html), "Content-Type: text/html");
    mg_send(nc, search_html, sizeof(search_html));
    nc->flags |= MG_F_SEND_AND_CLOSE;
}

int javascript(struct mg_connection *nc) {
    send_response_line(nc, 200, sizeof(bundle_js), "Content-Type: application/javascript");
    mg_send(nc, bundle_js, sizeof(bundle_js));
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

int style(struct mg_connection *nc, struct http_message *hm) {

    if (client_requested_dark_theme(hm)) {
        send_response_line(nc, 200, sizeof(bundle_dark_css), "Content-Type: text/css");
        mg_send(nc, bundle_dark_css, sizeof(bundle_dark_css));
    } else {
        send_response_line(nc, 200, sizeof(bundle_css), "Content-Type: text/css");
        mg_send(nc, bundle_css, sizeof(bundle_css));
    }

    nc->flags |= MG_F_SEND_AND_CLOSE;
}

int img_sprite_skin_flat(struct mg_connection *nc, struct http_message *hm) {
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
        LOG_DEBUGF("serve.c", "Invalid thumnail path: %.*s", (int) path->len, path->p);
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
        LOG_DEBUGF("serve.c", "Invalid thumbnail UUID: %s", arg_uuid);
        nc->flags |= MG_F_SEND_AND_CLOSE;
        return;
    }

    store_t *store = get_store(arg_index);
    if (store == NULL) {
        LOG_DEBUGF("serve.c", "Could not get store for index: %s", arg_index);
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

/**
 * Modified version of onion_shortcut_response_file that allows
 * browsers to seek media files.
 */
int chunked_response_file(const char *filename, const char *mime,
                          int partial, onion_request *request, onion_response *res) {
    int fd = open(filename, O_RDONLY | O_CLOEXEC);
    struct stat st;

    if (fd < 0 || stat(filename, &st) != 0 || S_ISDIR(st.st_mode)) {
        close(fd);
        return OCS_NOT_PROCESSED;
    }

    size_t length = st.st_size;
    size_t ends;

    const char *range = onion_request_get_header(request, "Range");
    if (partial && range && strncmp(range, "bytes=", 6) == 0) {
        onion_response_set_header(res, "Accept-Ranges", "bytes");

        onion_response_set_code(res, HTTP_PARTIAL_CONTENT);

        char tmp[1024];
        if (strlen(range + 6) >= sizeof(tmp)) {
            close(fd);
            return OCS_INTERNAL_ERROR;
        }
        strncpy(tmp, range + 6, sizeof(tmp) - 1);
        char *start = tmp;
        char *end = tmp;

        while (*end != '-' && *end) {
            end++;
        }

        if (*end == '-') {
            *end = '\0';
            end++;

            size_t starts;
            starts = atol(start);
            if (*end) {
                // %d-%d
                ends = atol(end);
            } else {
                // %d-
                ends = MIN(starts + CHUNK_SIZE, length);
            }
            if (ends > length || starts >= length || starts < 0) {
                close(fd);
                return OCS_INTERNAL_ERROR;
            }
            length = ends - starts;

            if (starts != 0) {
                lseek(fd, starts, SEEK_SET);
            }
            snprintf(tmp, sizeof(tmp), "bytes %ld-%ld/%ld",
                     starts, ends - 1, st.st_size);
            onion_response_set_header(res, "Content-Range", tmp);
        }
    }
    onion_response_set_length(res, length);
    if (mime != NULL) {
        onion_response_set_header(res, "Content-Type", mime);
    } else {
        onion_response_set_header(res, "Content-Type", "application/octet-stream");
    }

    onion_response_write_headers(res);
    if ((onion_request_get_flags(request) & OR_HEAD) == OR_HEAD) {
        length = 0;
    }

    if (length) {
        int bytes_read = 0, bytes_written;
        size_t total_read = 0;
        char buf[4046];
        if (length > sizeof(buf)) {
            size_t max = length - sizeof(buf);
            while (total_read < max) {
                bytes_read = read(fd, buf, sizeof(buf));
                if (bytes_read < 0) {
                    break;
                }
                total_read += bytes_read;
                bytes_written = onion_response_write(res, buf, bytes_read);
                if (bytes_written != bytes_read) {
                    break;
                }
            }
        }
        if (sizeof(buf) >= (length - total_read)) {
            bytes_read = read(fd, buf, length - total_read);
            onion_response_write(res, buf, bytes_read);
        }
    }
    close(fd);
    return OCS_PROCESSED;
}

void search(struct mg_connection *nc, struct http_message *hm) {

    if (hm->body.len == 0) {
        LOG_DEBUG("serve.c", "Client sent empty body, ignoring request")
        nc->flags |= MG_F_SEND_AND_CLOSE;
        return;
    }

    char * body = malloc(hm->body.len + 1);
    memcpy(body, hm->body.p, hm->body.len);
    *(body + hm->body.len) = '\0';

    char url[4096];
    snprintf(url, 4096, "%s/sist2/_search", WebCtx.es_url);

    nc->user_data = web_post_async(url, body);
    free(body);
}

int serve_file_from_url(cJSON *json, index_t *idx, onion_request *req, onion_response *res) {

    const char *path = cJSON_GetObjectItem(json, "path")->valuestring;
    const char *name = cJSON_GetObjectItem(json, "name")->valuestring;
    const char *ext = cJSON_GetObjectItem(json, "extension")->valuestring;

    char url[8196];
    snprintf(url, sizeof(url),
             "%s%s/%s%s%s",
             idx->desc.rewrite_url, path, name, strlen(ext) == 0 ? "" : ".", ext);

    dyn_buffer_t encoded = url_escape(url);
    int ret = onion_shortcut_redirect(encoded.buf, req, res);
    dyn_buffer_destroy(&encoded);
    return ret;
}

void serve_file_from_disk(cJSON *json, index_t *idx, struct mg_connection *nc, struct http_message *hm) {

    const char *path = cJSON_GetObjectItem(json, "path")->valuestring;
    const char *name = cJSON_GetObjectItem(json, "name")->valuestring;
    const char *ext = cJSON_GetObjectItem(json, "extension")->valuestring;
    const char *mime = cJSON_GetObjectItem(json, "mime")->valuestring;

    char full_path[PATH_MAX];
    snprintf(full_path, PATH_MAX, "%s%s%s%s%s%s",
             idx->desc.root, path, strlen(path) == 0 ? "" : "/",
             name, strlen(ext) == 0 ? "" : ".", ext);

    LOG_DEBUGF("serve.c", "Serving file from disk: %s", full_path)

    char disposition[8196];
    snprintf(disposition, sizeof(disposition), "Content-Disposition: inline; filename=\"%s%s%s\"",
             name, strlen(ext) == 0 ? "" : ".", ext);

    mg_http_serve_file(nc, hm, full_path, mg_mk_str(mime), mg_mk_str(disposition));
}

int index_info(struct mg_connection *nc) {
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


int document_info(UNUSED(void *p), onion_request *req, onion_response *res) {

    const char *arg_uuid = onion_request_get_query(req, "1");
    if (arg_uuid == NULL) {
        return OCS_PROCESSED;
    }

    cJSON *doc = elastic_get_document(arg_uuid);
    cJSON *source = cJSON_GetObjectItem(doc, "_source");

    cJSON *index_id = cJSON_GetObjectItem(source, "index");
    if (index_id == NULL) {
        cJSON_Delete(doc);
        return OCS_NOT_PROCESSED;
    }

    index_t *idx = get_index_by_id(index_id->valuestring);
    if (idx == NULL) {
        cJSON_Delete(doc);
        return OCS_NOT_PROCESSED;
    }

    onion_response_set_header(res, "Content-Type", "application/json");

    char *json_str = cJSON_PrintUnformatted(source);
    onion_response_write0(res, json_str);
    free(json_str);
    cJSON_Delete(doc);

    return OCS_PROCESSED;
}

void file(struct mg_connection *nc, struct http_message *hm, struct mg_str *path) {

    if (path->len != UUID_STR_LEN + 2) {
        LOG_DEBUGF("serve.c", "Invalid file path: %.*s", (int) path->len, path->p);
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
        return;
    }

    if (strlen(idx->desc.rewrite_url) == 0) {
        serve_file_from_disk(source, idx, nc, hm);
    } else {
        //TODO:
//        serve_file_from_url(source, idx, nc, hm);
    }
    cJSON_Delete(doc);
}

int status(UNUSED(void *p), UNUSED(onion_request *req), onion_response *res) {
    onion_response_set_header(res, "Content-Type", "application/x-empty");

    char *status = elastic_get_status();
    if (strcmp(status, "open") == 0) {
        onion_response_set_code(res, 204);
    } else {
        onion_response_set_code(res, 500);
    }

    free(status);

    return OCS_PROCESSED;
}

typedef struct {
    void *resp;
} req_ctx_t;

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
            nc->flags |= MG_F_SEND_AND_CLOSE;
            return;
        }

        if (is_equal(&path, &((struct mg_str) MG_MK_STR("/")))) {
            search_index(nc);
        } else if (is_equal(&path, &((struct mg_str) MG_MK_STR("/css")))) {
            style(nc, hm);
        } else if (is_equal(&path, &((struct mg_str) MG_MK_STR("/js")))) {
            javascript(nc);
        } else if (is_equal(&path, &((struct mg_str) MG_MK_STR("/img/sprite-skin-flat.png")))) {
            img_sprite_skin_flat(nc, hm);
        } else if (is_equal(&path, &((struct mg_str) MG_MK_STR("/es")))) {
            search(nc, hm);
        } else if (is_equal(&path, &((struct mg_str) MG_MK_STR("/i")))) {
            index_info(nc);
        } else if (has_prefix(&path, &((struct mg_str) MG_MK_STR("/f/")))) {
            file(nc, hm, &path);
        } else if (has_prefix(&path, &((struct mg_str) MG_MK_STR("/t/")))) {
            thumbnail(nc, hm, &path);
        } else {

            nc->flags |= MG_F_SEND_AND_CLOSE;
        }


    } else if (ev == MG_EV_POLL) {
        if (nc->user_data != NULL) {
            //Waiting for ES reply
            subreq_ctx_t *ctx = (subreq_ctx_t*) nc->user_data;
            mg_mgr_poll(&ctx->mgr, 0);

            if (ctx->ev_data.done == TRUE) {

                response_t *r = ctx->ev_data.resp;

                if (r->status_code == 200) {
                    send_response_line(nc, 200, r->size, "Content-Type: application/json");
                    mg_send(nc, r->body, r->size);
                } else {
                    fprintf(stderr, "ERR \n%s", r->body);
                    sist_log("serve.c", SIST_WARNING, "ElasticSearch error during query");
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
                    //todo return error code
                }

                free_response(r);
                nc->flags |= MG_F_SEND_AND_CLOSE;
                nc->user_data = NULL;
            }
        }
    }
}

void serve(const char *hostname, const char *port) {

    struct mg_mgr mgr;
    mg_mgr_init(&mgr, NULL);

    struct mg_connection *nc = mg_bind(&mgr, "8000", ev_router);
    if (nc == NULL) {
        printf("Failed to create listener\n");
        return;
    }
    mg_set_protocol_http_websocket(nc);

    for (;;) {
        mg_mgr_poll(&mgr, 10);
    }

    onion *o = onion_new(O_POOL);
    onion_set_timeout(o, 3500);

    onion_set_hostname(o, hostname);
    onion_set_port(o, port);

    onion_url *urls = onion_url_new();

    // Static paths
    onion_set_root_handler(o, auth_basic(WebCtx.b64credentials, onion_url_to_handler(urls)));

    onion_url_add(urls, "", search_index);
    onion_url_add(urls, "css", style);
    onion_url_add(urls, "js", javascript);
    onion_url_add(urls, "img/sprite-skin-flat.png", img_sprite_skin_flat);

    onion_url_add(urls, "es", search);
    onion_url_add(urls, "status", status);
    onion_url_add(
            urls,
            "^t/([a-fA-F0-9]{8}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{12})/"
            "([a-fA-F0-9]{8}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{12})$",
            thumbnail
    );
    onion_url_add(urls, "^f/([a-fA-F0-9]{8}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{12})$", file);
    onion_url_add(urls, "^d/([a-fA-F0-9]{8}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{12})$",
                  document_info);
    onion_url_add(urls, "i", index_info);


    printf("Starting web server @ http://%s:%s\n", hostname, port);

    onion_listen(o);
    onion_free(o);
}
