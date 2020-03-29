#include "web.h"
#include "src/sist.h"
#include "src/ctx.h"

#include <mongoose.h>
#include <pthread.h>


size_t write_cb(char *ptr, size_t size, size_t nmemb, void *user_data) {

    size_t real_size = size * nmemb;
    dyn_buffer_t *buf = user_data;
    dyn_buffer_write(buf, ptr, real_size);
    return real_size;
}

void free_response(response_t *resp) {
    free(resp->body);
    free(resp);
}

#define SIST2_HEADERS "User-Agent: sist2\r\nContent-Type: application/json\r\n"


typedef struct {
    response_t *resp;
    int done;
} http_ev_data_t;

void http_req_ev(struct mg_connection *nc, int ev, void *ptr) {

    http_ev_data_t *ev_data = (http_ev_data_t *) nc->user_data;

    switch (ev) {
        case MG_EV_CONNECT: {
            int connect_status = *(int *) ptr;
            if (connect_status != 0) {
                printf("ERROR connecting\n");
                ev_data->done = TRUE;
            }
            printf("EV_CONNECT: %d\n", connect_status);
            break;
        }
        case MG_EV_HTTP_REPLY: {
            struct http_message *hm = (struct http_message *) ptr;
            printf("Got reply: %d\n", (int) hm->body.len);

            nc->flags |= MG_F_SEND_AND_CLOSE;

            ev_data->resp->size = hm->body.len;
            ev_data->resp->status_code = hm->resp_code;
            ev_data->resp->body = malloc(hm->body.len);
            memcpy(ev_data->resp->body, hm->body.p, hm->body.len);

            ev_data->done = TRUE;
            break;
        }
        case MG_EV_CLOSE: {
            printf("Server closed connection\n");
            ev_data->done = TRUE;
            break;
        }
        default:
            break;
    }
}

response_t *http_req(const char *url, const char *extra_headers, const char *post_data, const char *method) {

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

    if (mg_parse_uri(mg_mk_str(url), &scheme, &user_info, &host, &port, &path, &query, &fragment) != 0) {
        LOG_ERRORF("web.c", "Could not parse URL: %s", url)
        return NULL;
    }

    http_ev_data_t ev_data;
    ev_data.resp = malloc(sizeof(response_t));
    ev_data.done = FALSE;

    struct mg_mgr mgr;
    mg_mgr_init(&mgr, NULL);

    url = "tcp://localhost:9200";
    struct mg_connection *conn = mg_connect(&mgr, url, http_req_ev);
    conn->user_data = &ev_data;
    mg_set_protocol_http_websocket(conn);

    mg_printf(
            conn, "%s %.*s HTTP/1.1\r\n"
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

    while (ev_data.done == FALSE) {
        mg_mgr_poll(&mgr, 50);
    }

    return ev_data.resp;
}

response_t *web_get(const char *url) {
    return http_req(url, SIST2_HEADERS, NULL, "GET");
}

response_t *web_post(const char *url, const char *data, const char *header) {
    return http_req(url, SIST2_HEADERS, data, "POST");
}


response_t *web_put(const char *url, const char *data, const char *header) {
    return http_req(url, SIST2_HEADERS, data, "PUT");
}

response_t *web_delete(const char *url) {
    return http_req(url, SIST2_HEADERS, NULL, "DELETE");
}
