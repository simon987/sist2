#ifndef SIST2_ELASTIC_H
#define SIST2_ELASTIC_H

#include "src/sist.h"

typedef struct es_bulk_line {
    struct es_bulk_line *next;
    char path_md5_str[MD5_STR_LENGTH];
    char line[0];
} es_bulk_line_t;

/**
 * Note: indexer is *not* thread safe
 */
typedef struct es_indexer es_indexer_t;

void elastic_index_line(es_bulk_line_t *line);

void print_json(cJSON *document, const char index_id_str[MD5_STR_LENGTH]);

void index_json(cJSON *document, const char index_id_str[MD5_STR_LENGTH]);

es_indexer_t *create_indexer(const char *url, const char *index);

void elastic_cleanup();
void finish_indexer(char *script, int async_script, char *index_id);

void elastic_init(int force_reset, const char* user_mappings, const char* user_settings);

cJSON *elastic_get_document(const char *id_str);

char *elastic_get_status();

void execute_update_script(const char *script, int async, const char index_id[MD5_STR_LENGTH]);

#endif
