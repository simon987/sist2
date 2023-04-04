#include "web_util.h"
#include "static_generated.c"


void web_serve_asset_index_html(struct mg_connection *nc) {
    web_send_headers(nc, 200, sizeof(index_html), "Content-Type: text/html");
    mg_send(nc, index_html, sizeof(index_html));
}

void web_serve_asset_index_js(struct mg_connection *nc) {
    web_send_headers(nc, 200, sizeof(index_js), "Content-Type: application/javascript");
    mg_send(nc, index_js, sizeof(index_js));
}

void web_serve_asset_chunk_vendors_js(struct mg_connection *nc) {
    web_send_headers(nc, 200, sizeof(chunk_vendors_js), "Content-Type: application/javascript");
    mg_send(nc, chunk_vendors_js, sizeof(chunk_vendors_js));
}

void web_serve_asset_favicon_ico(struct mg_connection *nc) {
    web_send_headers(nc, 200, sizeof(favicon_ico), "Content-Type: image/x-icon");
    mg_send(nc, favicon_ico, sizeof(favicon_ico));
}

void web_serve_asset_style_css(struct mg_connection *nc) {
    web_send_headers(nc, 200, sizeof(index_css), "Content-Type: text/css");
    mg_send(nc, index_css, sizeof(index_css));
}

void web_serve_asset_chunk_vendors_css(struct mg_connection *nc) {
    web_send_headers(nc, 200, sizeof(chunk_vendors_css), "Content-Type: text/css");
    mg_send(nc, chunk_vendors_css, sizeof(chunk_vendors_css));
}

index_t *web_get_index_by_id(const char *index_id) {
    for (int i = WebCtx.index_count; i >= 0; i--) {
        if (strncmp(index_id, WebCtx.indices[i].desc.id, SIST_INDEX_ID_LEN) == 0) {
            return &WebCtx.indices[i];
        }
    }
    return NULL;
}

database_t *web_get_database(const char *index_id) {
    index_t *idx = web_get_index_by_id(index_id);
    if (idx != NULL) {
        return idx->db;
    }
    return NULL;
}

void web_send_headers(struct mg_connection *nc, int status_code, size_t length, char *extra_headers) {
    mg_printf(
            nc,
            "HTTP/1.1 %d %s\r\n"
    HTTP_SERVER_HEADER
    "Content-Length: %d\r\n"
    "%s\r\n\r\n",
            status_code, "OK",
            length,
            extra_headers
    );
}
