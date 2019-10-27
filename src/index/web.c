#include "web.h"

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

response_t *web_get(const char *url) {
    response_t *resp = malloc(sizeof(response_t));

    CURL *curl;
    dyn_buffer_t buffer = dyn_buffer_create();

    curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) (&buffer));
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "sist2");

    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &resp->status_code);

    curl_easy_cleanup(curl);

    resp->body = buffer.buf;
    resp->size = buffer.cur;
    return resp;
}

response_t *web_post(const char *url, const char *data, const char *header) {

    response_t *resp = malloc(sizeof(response_t));

    CURL *curl;
    dyn_buffer_t buffer = dyn_buffer_create();

    curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) (&buffer));
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_POST, 1);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "sist2");

    struct curl_slist *headers = NULL;
    if (header != NULL) {
        headers = curl_slist_append(headers, header);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    }

    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);

    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &resp->status_code);

    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);

    resp->body = buffer.buf;
    resp->size = buffer.cur;

    return resp;
}


response_t *web_put(const char *url, const char *data, const char *header) {

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

    if (header != NULL) {
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, header);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    }

    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);

    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &resp->status_code);

    curl_easy_cleanup(curl);

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

    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &resp->status_code);

    curl_easy_cleanup(curl);

    resp->body = buffer.buf;
    resp->size = buffer.cur;
    return resp;
}
