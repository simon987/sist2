#ifndef SIST2_SERVE_H
#define SIST2_SERVE_H

#include "src/sist.h"

#define HTTP_TEXT_TYPE_HEADER "Content-Type: text/plain;charset=utf-8\r\n"
#define HTTP_REPLY_NOT_FOUND mg_http_reply(nc, 404, HTTP_SERVER_HEADER HTTP_TEXT_TYPE_HEADER, "Not found");
#define HTTP_REPLY_BAD_REQUEST mg_http_reply(nc, 400, HTTP_SERVER_HEADER HTTP_TEXT_TYPE_HEADER, "Invalid request");
#define HTTP_REPLY_OK mg_http_reply(nc, 200, HTTP_SERVER_HEADER HTTP_TEXT_TYPE_HEADER, "ok");

void serve(const char *listen_address);

#endif
