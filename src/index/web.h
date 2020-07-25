#ifndef SIST2_WEB_H
#define SIST2_WEB_H

#include "src/sist.h"
#include <mongoose.h>

typedef struct response {
    char *body;
    size_t size;
    int status_code;
} response_t;

typedef struct {
    response_t *resp;
    int done;
} http_ev_data_t;

typedef struct {
    http_ev_data_t ev_data;
    struct mg_mgr mgr;
} subreq_ctx_t;

response_t *web_get(const char *url, int timeout);
response_t *web_post(const char * url, const char * data);
subreq_ctx_t *web_post_async(const char *url, const char *data);
response_t *web_put(const char *url, const char *data);
response_t *web_delete(const char *url);

void free_response(response_t *resp);

#endif
