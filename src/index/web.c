#include "web.h"
#include "src/sist.h"

#include <mongoose.h>
#include <pthread.h>
#include <curl/curl.h>


size_t write_cb(char *ptr, size_t size, size_t nmemb, void *user_data) {

    size_t real_size = size * nmemb;
    dyn_buffer_t *buf = user_data;
    dyn_buffer_write(buf, ptr, real_size);
    return real_size;
}

void free_response(response_t *resp) {
    if (resp->body != NULL) {
        free(resp->body);
    }
    free(resp);
}

#define SIST2_HEADERS "User-Agent: sist2\r\nContent-Type: application/json\r\n"


void http_req_ev(struct mg_connection *nc, int ev, void *ptr) {

    http_ev_data_t *ev_data = (http_ev_data_t *) nc->user_data;

    switch (ev) {
        case MG_EV_CONNECT: {
            int connect_status = *(int *) ptr;
            if (connect_status != 0) {
                ev_data->done = TRUE;
                ev_data->resp->status_code = 0;
            }
            break;
        }
        case MG_EV_HTTP_REPLY: {
            struct http_message *hm = (struct http_message *) ptr;

            //TODO: Check errors?

            ev_data->resp->size = hm->body.len;
            ev_data->resp->status_code = hm->resp_code;
            ev_data->resp->body = malloc(hm->body.len + 1);
            memcpy(ev_data->resp->body, hm->body.p, hm->body.len);
            *(ev_data->resp->body + hm->body.len) = '\0';

            ev_data->done = TRUE;
            break;
        }
        case MG_EV_CLOSE: {
            ev_data->done = TRUE;
            break;
        }
        default:
            break;
    }
}

subreq_ctx_t *http_req(const char *url, const char *extra_headers, const char *post_data, const char *method) {

    struct mg_str scheme;
    struct mg_str user_info;
    struct mg_str host;
    unsigned int port;
    struct mg_str path;
    struct mg_str query;
    struct mg_str fragment;

    if (post_data == NULL) post_data = "";
    if (extra_headers == NULL) extra_headers = "";
    if (path.len == 0) path = mg_mk_str("/");
    if (host.len == 0) host = mg_mk_str("");

    // [scheme://[user_info@]]host[:port][/path][?query][#fragment]
    mg_parse_uri(mg_mk_str(url), &scheme, &user_info, &host, &port, &path, &query, &fragment);

    if (query.len > 0) path.len += query.len + 1;

    subreq_ctx_t *ctx = malloc(sizeof(subreq_ctx_t));
    mg_mgr_init(&ctx->mgr, NULL);

    char address[8196];
    snprintf(address, sizeof(address), "tcp://%.*s:%u", (int) host.len, host.p, port);
    struct mg_connection *nc = mg_connect(&ctx->mgr, address, http_req_ev);
    nc->user_data = &ctx->ev_data;
    mg_set_protocol_http_websocket(nc);

    ctx->ev_data.resp = calloc(1, sizeof(response_t));
    ctx->ev_data.done = FALSE;

    mg_printf(
            nc, "%s %.*s HTTP/1.1\r\n"
                  "Host: %.*s\r\n"
                  "Content-Length: %zu\r\n"
                  "%s\r\n"
                  "%s",
            method, (int) path.len, path.p,
            (int) (path.p - host.p), host.p,
            strlen(post_data),
            extra_headers,
            post_data
    );

    return ctx;
}

subreq_ctx_t *web_post_async(const char *url, const char *data) {
    return http_req(url, SIST2_HEADERS, data, "POST");
}

response_t *web_get(const char *url, int timeout) {
    response_t *resp = malloc(sizeof(response_t));

    CURL *curl;
    dyn_buffer_t buffer = dyn_buffer_create();

    curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) (&buffer));
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "sist2");
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout);

    struct curl_slist *headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &resp->status_code);

    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);

    resp->body = buffer.buf;
    resp->size = buffer.cur;
    return resp;
}

response_t *web_post(const char *url, const char *data) {

    response_t *resp = malloc(sizeof(response_t));

    CURL *curl;
    dyn_buffer_t buffer = dyn_buffer_create();

    curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) (&buffer));
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_POST, 1);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "sist2");

    struct curl_slist *headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);

    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &resp->status_code);

    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);

    resp->body = buffer.buf;
    resp->size = buffer.cur;

    return resp;
}


response_t *web_put(const char *url, const char *data) {

    response_t *resp = malloc(sizeof(response_t));

    CURL *curl;
    dyn_buffer_t buffer = dyn_buffer_create();

    curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) (&buffer));
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "sist2");
    curl_easy_setopt(curl, CURLOPT_DNS_USE_GLOBAL_CACHE, 0);
    curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURLOPT_DNS_LOCAL_IP4 );

    struct curl_slist *headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);

    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &resp->status_code);

    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);

    resp->body = buffer.buf;
    resp->size = buffer.cur;
    return resp;
}

response_t *web_delete(const char *url) {

    response_t *resp = malloc(sizeof(response_t));

    CURL *curl;
    dyn_buffer_t buffer = dyn_buffer_create();

    curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) (&buffer));
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "sist2");

    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "");
    struct curl_slist *headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &resp->status_code);

    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);

    resp->body = buffer.buf;
    resp->size = buffer.cur;
    return resp;
}