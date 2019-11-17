#import "auth_basic.h"

#define UNAUTHORIZED_TEXT "Unauthorized"

typedef struct auth_basic_data {
    onion_handler *inside;
    const char *b64credentials;
} auth_basic_data_t;


int authenticate(const char *expected, const char *credentials) {

    if (expected == NULL) {
        return TRUE;
    }

    if (credentials && strncmp(credentials, "Basic ", 6) == 0) {
        if (strcmp((credentials + 6), expected) == 0) {
            return TRUE;
        }
    }

    return FALSE;
}

int auth_basic_handler(auth_basic_data_t *d,
                       onion_request *req,
                       onion_response *res) {

    const char *credentials = onion_request_get_header(req, "Authorization");

    if (authenticate(d->b64credentials, credentials)) {
        return onion_handler_handle(d->inside, req, res);
    }

    onion_response_set_header(res, "WWW-Authenticate", "Basic realm=\"sist2\"");
    onion_response_set_code(res, HTTP_UNAUTHORIZED);
    onion_response_write(res, UNAUTHORIZED_TEXT, sizeof(UNAUTHORIZED_TEXT));
    onion_response_set_length(res, sizeof(UNAUTHORIZED_TEXT));

    return OCS_PROCESSED;
}

void auth_basic_free(auth_basic_data_t *data) {
    onion_handler_free(data->inside);
    free(data);
}

onion_handler *auth_basic(const char *b64credentials, onion_handler *inside_level) {

    auth_basic_data_t *privdata = malloc(sizeof(auth_basic_data_t));

    privdata->b64credentials = b64credentials;
    privdata->inside = inside_level;

    return onion_handler_new((onion_handler_handler) auth_basic_handler, privdata,
                             (onion_handler_private_data_free) auth_basic_free);
}

