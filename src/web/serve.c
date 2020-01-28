#include "serve.h"
#include <src/ctx.h>
#include <onion/types_internal.h>

#include "static_generated.c"

#define CHUNK_SIZE 1024 * 1024 * 10

__always_inline
void set_default_headers(onion_response *res) {
    onion_response_set_header(res, "Server", "sist2");
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

int search_index(void *p, onion_request *req, onion_response *res) {
    set_default_headers(res);
    onion_response_set_header(res, "Content-Type", "text/html");
    onion_response_set_length(res, sizeof(search_html));
    onion_response_write(res, search_html, sizeof(search_html));
    return OCS_PROCESSED;
}

int javascript(void *p, onion_request *req, onion_response *res) {
    onion_response_set_header(res, "Content-Type", "text/javascript");
    onion_response_set_length(res, sizeof(bundle_js));
    onion_response_write(res, bundle_js, sizeof(bundle_js));
    return OCS_PROCESSED;
}

int client_requested_dark_theme(onion_request *req) {
    const char *cookie = onion_request_get_cookie(req, "sist");
    if (cookie == NULL) {
        return FALSE;
    }

    return strcmp(cookie, "dark") == 0;
}

int style(void *p, onion_request *req, onion_response *res) {
    set_default_headers(res);

    onion_response_set_header(res, "Content-Type", "text/css");

    if (client_requested_dark_theme(req)) {
        onion_response_set_length(res, sizeof(bundle_dark_css));
        onion_response_write(res, bundle_dark_css, sizeof(bundle_dark_css));
    } else {
        onion_response_set_length(res, sizeof(bundle_css));
        onion_response_write(res, bundle_css, sizeof(bundle_css));
    }
    return OCS_PROCESSED;
}

int img_sprite_skin_flag(void *p, onion_request *req, onion_response *res) {
    set_default_headers(res);
    onion_response_set_header(res, "Content-Type", "image/png");
    if (client_requested_dark_theme(req)) {
        onion_response_set_length(res, sizeof(sprite_skin_flat_dark_png));
        onion_response_write(res, sprite_skin_flat_dark_png, sizeof(sprite_skin_flat_dark_png));
    } else {
        onion_response_set_length(res, sizeof(sprite_skin_flat_png));
        onion_response_write(res, sprite_skin_flat_png, sizeof(sprite_skin_flat_png));
    }
    return OCS_PROCESSED;
}

int thumbnail(void *p, onion_request *req, onion_response *res) {
    int flags = onion_request_get_flags(req);
    if ((flags & OR_METHODS) != OR_GET) {
        return OCS_NOT_PROCESSED;
    }
    const char *arg_index = onion_request_get_query(req, "1");
    const char *arg_uuid = onion_request_get_query(req, "2");

    if (arg_uuid == NULL || arg_index == NULL) {
        return OCS_NOT_PROCESSED;
    }

    uuid_t uuid;
    uuid_parse(arg_uuid, uuid);

    store_t *store = get_store(arg_index);

    if (store == NULL) {
        return OCS_NOT_PROCESSED;
    }

    set_default_headers(res);
    onion_response_set_header(res, "Content-Type", "image/jpeg");

    size_t data_len = 0;
    char *data = store_read(store, (char *) uuid, sizeof(uuid_t), &data_len);
    onion_response_set_length(res, data_len);
    int written = onion_response_write(res, data, data_len);
    onion_response_flush(res);
    if (written != data_len || data_len == 0) {
        LOG_DEBUG("serve.c", "Couldn't write thumbnail");
    }
    free(data);

    return OCS_PROCESSED;
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
    onion_response_set_header(res, "Content-Type", mime);
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

int search(UNUSED(void *p), onion_request *req, onion_response *res) {

    int flags = onion_request_get_flags(req);
    if ((flags & OR_METHODS) != OR_POST) {
        return OCS_NOT_PROCESSED;
    }

    char *scroll_param;
    const char *scroll = onion_request_get_query(req, "scroll");
    if (scroll != NULL) {
        scroll_param = "?scroll=3m";
    } else {
        scroll_param = "";
    }

    const struct onion_block_t *block = onion_request_get_data(req);

    if (block == NULL) {
        return OCS_NOT_PROCESSED;
    }

    char url[4096];
    snprintf(url, 4096, "%s/sist2/_search%s", WebCtx.es_url, scroll_param);
    response_t *r = web_post(url, onion_block_data(block), "Content-Type: application/json");

    set_default_headers(res);
    onion_response_set_header(res, "Content-Type", "application/json");
    onion_response_set_length(res, r->size);

    if (r->status_code == 200) {
        onion_response_write(res, r->body, r->size);
    } else {
        onion_response_set_code(res, HTTP_INTERNAL_ERROR);
    }

    free_response(r);

    return OCS_PROCESSED;
}

int scroll(UNUSED(void *p), onion_request *req, onion_response *res) {

    int flags = onion_request_get_flags(req);
    if ((flags & OR_METHODS) != OR_GET) {
        return OCS_NOT_PROCESSED;
    }

    char url[4096];
    snprintf(url, 4096, "%s/_search/scroll", WebCtx.es_url);

    const char *scroll_id = onion_request_get_query(req, "scroll_id");

    cJSON *json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "scroll_id", scroll_id);
    cJSON_AddStringToObject(json, "scroll", "3m");

    char *json_str = cJSON_PrintUnformatted(json);
    response_t *r = web_post(url, json_str, "Content-Type: application/json");

    cJSON_Delete(json);
    cJSON_free(json_str);

    if (r->status_code != 200) {
        free_response(r);
        return OCS_NOT_PROCESSED;
    }

    set_default_headers(res);
    onion_response_set_header(res, "Content-Type", "application/json");
    onion_response_set_header(res, "Content-Disposition", "application/json");
    onion_response_set_length(res, r->size);
    onion_response_write(res, r->body, r->size);
    free_response(r);

    return OCS_PROCESSED;
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

int serve_file_from_disk(cJSON *json, index_t *idx, onion_request *req, onion_response *res) {

    const char *path = cJSON_GetObjectItem(json, "path")->valuestring;
    const char *name = cJSON_GetObjectItem(json, "name")->valuestring;
    const char *ext = cJSON_GetObjectItem(json, "extension")->valuestring;
    const char *mime = cJSON_GetObjectItem(json, "mime")->valuestring;

    char full_path[PATH_MAX];
    snprintf(full_path, PATH_MAX, "%s%s/%s%s%s",
             idx->desc.root, path, name, strlen(ext) == 0 ? "" : ".", ext);

    char disposition[8196];
    snprintf(disposition, sizeof(disposition), "inline; filename=\"%s%s%s\"",
             name, strlen(ext) == 0 ? "" : ".", ext);
    onion_response_set_header(res, "Content-Disposition", disposition);

    return chunked_response_file(full_path, mime, 1, req, res);
}

int index_info(UNUSED(void *p), onion_request *req, onion_response *res) {
    cJSON *json = cJSON_CreateObject();
    cJSON *arr = cJSON_AddArrayToObject(json, "indices");

    set_default_headers(res);
    onion_response_set_header(res, "Content-Type", "application/json");

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
    onion_response_write0(res, json_str);
    free(json_str);
    cJSON_Delete(json);

    return OCS_PROCESSED;
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

int file(UNUSED(void *p), onion_request *req, onion_response *res) {

    const char *arg_uuid = onion_request_get_query(req, "1");
    if (arg_uuid == NULL) {
        return OCS_PROCESSED;
    }

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
            return OCS_NOT_PROCESSED;
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
        return OCS_NOT_PROCESSED;
    }

    int ret;
    if (strlen(idx->desc.rewrite_url) == 0) {
        ret = serve_file_from_disk(source, idx, req, res);
    } else {
        ret = serve_file_from_url(source, idx, req, res);
    }
    cJSON_Delete(doc);

    return ret;
}

int status(UNUSED(void *p), UNUSED(onion_request *req), onion_response *res) {
    set_default_headers(res);

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

void serve(const char *hostname, const char *port) {
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
    onion_url_add(urls, "img/sprite-skin-flat.png", img_sprite_skin_flag);

    onion_url_add(urls, "es", search);
    onion_url_add(urls, "scroll", scroll);
    onion_url_add(urls, "status", status);
    onion_url_add(
            urls,
            "^t/([a-fA-F0-9]{8}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{12})/"
            "([a-fA-F0-9]{8}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{12})$",
            thumbnail
    );
    onion_url_add(urls, "^f/([a-fA-F0-9]{8}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{12})$", file);
    onion_url_add(urls, "^d/([a-fA-F0-9]{8}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{12})$", document_info);
    onion_url_add(urls, "i", index_info);


    printf("Starting web server @ http://%s:%s\n", hostname, port);

    onion_listen(o);
    onion_free(o);
}
