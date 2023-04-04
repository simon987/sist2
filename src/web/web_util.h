#ifndef SIST2_WEB_UTIL_H
#define SIST2_WEB_UTIL_H

#include "src/sist.h"
#include "src/index/elastic.h"
#include "src/ctx.h"
#include <mongoose.h>

#define HTTP_SERVER_HEADER "Server: sist2/" VERSION "\r\n"

index_t *web_get_index_by_id(const char *index_id);

database_t *web_get_database(const char *index_id);

__always_inline
static char *web_address_to_string(struct mg_addr *addr) {
    return "TODO";
//    static char address_to_string_buf[INET6_ADDRSTRLEN];
//
//    return mg_ntoa(addr, address_to_string_buf, sizeof(address_to_string_buf));
}

void web_send_headers(struct mg_connection *nc, int status_code, size_t length, char *extra_headers);

void web_serve_asset_index_html(struct mg_connection *nc);
void web_serve_asset_index_js(struct mg_connection *nc);
void web_serve_asset_chunk_vendors_js(struct mg_connection *nc);
void web_serve_asset_favicon_ico(struct mg_connection *nc);
void web_serve_asset_style_css(struct mg_connection *nc);
void web_serve_asset_chunk_vendors_css(struct mg_connection *nc);

#endif //SIST2_WEB_UTIL_H
