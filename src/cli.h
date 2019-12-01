#ifndef SIST2_CLI_H
#define SIST2_CLI_H

#include "sist.h"

typedef struct scan_args {
    float quality;
    int size;
    int content_size;
    int threads;
    char *incremental;
    char *output;
    char *rewrite_url;
    char *name;
    int depth;
    char *path;
} scan_args_t;

scan_args_t *scan_args_create();
void scan_args_destroy(scan_args_t *args);
int scan_args_validate(scan_args_t *args, int argc, const char **argv);

#ifndef SIST_SCAN_ONLY
typedef struct index_args {
    char *es_url;
    const char *index_path;
    const char *script_path;
    char *script;
    int print;
    int force_reset;
} index_args_t;

typedef struct web_args {
    char *es_url;
    char *bind;
    char *port;
    char *credentials;
    char *b64credentials;
    int index_count;
    const char **indices;
} web_args_t;

index_args_t *index_args_create();
void index_args_destroy(index_args_t *args);

web_args_t *web_args_create();
void web_args_destroy(web_args_t *args);

int index_args_validate(index_args_t *args, int argc, const char **argv);
int web_args_validate(web_args_t *args, int argc, const char **argv);
#endif

#endif
