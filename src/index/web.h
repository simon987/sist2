#ifndef SIST2_WEB_H
#define SIST2_WEB_H

#include "src/sist.h"

typedef struct response {
    char *body;
    size_t size;
    int status_code;
} response_t;

response_t *web_get(const char *url);
response_t *web_post(const char * url, const char * data, const char* header);
response_t *web_put(const char *url, const char *data, const char *header);
response_t *web_delete(const char *url);

void free_response(response_t *resp);

#endif
