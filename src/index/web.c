#include "web.h"
#include "src/sist.h"
#include "src/ctx.h"

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

void web_post_async_poll(subreq_ctx_t* req) {
    fd_set fdread;
    fd_set fdwrite;
    fd_set fdexcep;
    int maxfd = -1;

    FD_ZERO(&fdread);
    FD_ZERO(&fdwrite);
    FD_ZERO(&fdexcep);

    CURLMcode mc = curl_multi_fdset(req->multi, &fdread, &fdwrite, &fdexcep, &maxfd);

    if(mc != CURLM_OK) {
        req->done = TRUE;
        return;
    }

    if (maxfd == -1) {
        // no fds ready yet
        return;
    }

    struct timeval timeout = {1, 0};
    int rc = select(maxfd + 1, &fdread, &fdwrite, &fdexcep, &timeout);

    switch(rc) {
        case -1:
            req->done = TRUE;
            break;
        case 0:
            break;
        default:
            curl_multi_perform(req->multi, &req->running_handles);
            break;
    }

    if (req->running_handles == 0) {
        req->done = TRUE;
        req->response->body = req->response_buf.buf;
        req->response->size = req->response_buf.cur;
        curl_easy_getinfo(req->handle, CURLINFO_RESPONSE_CODE, &req->response->status_code);

        curl_multi_cleanup(req->multi);
        curl_easy_cleanup(req->handle);
        curl_slist_free_all(req->headers);
        return;
    }
}

subreq_ctx_t *web_post_async(const char *url, char *data) {
    subreq_ctx_t *req = calloc(1, sizeof(subreq_ctx_t));
    req->response = calloc(1, sizeof(response_t));
    req->data = data;
    req->response_buf = dyn_buffer_create();

    req->handle = curl_easy_init();
    CURL *curl = req->handle;
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) (&req->response_buf));
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_POST, 1);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "sist2");

    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);

    req->multi = curl_multi_init();
    curl_multi_add_handle(req->multi, curl);
    curl_multi_perform(req->multi, &req->running_handles);

    LOG_DEBUGF("web.c", "async request POST %s", url)

    return req;
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

    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
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

    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
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

    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
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
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &resp->status_code);

    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);

    resp->body = buffer.buf;
    resp->size = buffer.cur;
    return resp;
}