#ifndef SIST2_CLI_H
#define SIST2_CLI_H

#include "sist.h"

#include "libscan/arc/arc.h"

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
    char *archive;
    archive_mode_t archive_mode;
    char *tesseract_lang;
    const char *tesseract_path;
    char *exclude_regex;
    int fast;
    const char* treemap_threshold_str;
    double treemap_threshold;
    int max_memory_buffer;
} scan_args_t;

scan_args_t *scan_args_create();

void scan_args_destroy(scan_args_t *args);

int scan_args_validate(scan_args_t *args, int argc, const char **argv);

typedef struct index_args {
    char *es_url;
    char *es_index;
    const char *index_path;
    const char *script_path;
    char *script;
    const char *es_settings_path;
    char *es_settings;
    const char *es_mappings_path;
    char *es_mappings;
    int print;
    int batch_size;
    int async_script;
    int force_reset;
    int threads;
} index_args_t;

typedef struct web_args {
    char *es_url;
    char *es_index;
    char *listen_address;
    char *credentials;
    char *tag_credentials;
    char auth_user[256];
    char auth_pass[256];
    int auth_enabled;
    int tag_auth_enabled;
    int index_count;
    const char **indices;
} web_args_t;

typedef struct exec_args {
    char *es_url;
    char *es_index;
    const char *index_path;
    const char *script_path;
    int async_script;
    char *script;
} exec_args_t;

index_args_t *index_args_create();

void index_args_destroy(index_args_t *args);

web_args_t *web_args_create();

void web_args_destroy(web_args_t *args);

int index_args_validate(index_args_t *args, int argc, const char **argv);

int web_args_validate(web_args_t *args, int argc, const char **argv);

exec_args_t *exec_args_create();

void exec_args_destroy(exec_args_t *args);

int exec_args_validate(exec_args_t *args, int argc, const char **argv);

#endif
