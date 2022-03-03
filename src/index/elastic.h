#ifndef SIST2_ELASTIC_H
#define SIST2_ELASTIC_H

#include "src/sist.h"

#define ES_BULK_LINE_INDEX 0
#define ES_BULK_LINE_DELETE 1

typedef struct es_bulk_line {
    struct es_bulk_line *next;
    char doc_id[SIST_DOC_ID_LEN];
    int type;
    char line[0];
} es_bulk_line_t;

typedef struct {
    int major;
    int minor;
    int patch;
} es_version_t;

#define VERSION_GE(version, maj, min) ((version)->major > (maj) || ((version)->major == (maj) && (version)->minor >= (min)))
#define IS_SUPPORTED_ES_VERSION(es_version) VERSION_GE((es_version), 6, 8)
#define USE_LEGACY_ES_SETTINGS(es_version) (!VERSION_GE((es_version), 7, 14))

__always_inline
static const char *format_es_version(es_version_t *version) {
    static char buf[64];

    snprintf(buf, sizeof(buf), "%d.%d.%d", version->major, version->minor, version->patch);

    return buf;
}


/**
 * Note: indexer is *not* thread safe
 */
typedef struct es_indexer es_indexer_t;

void elastic_index_line(es_bulk_line_t *line);

void print_json(cJSON *document, const char index_id_str[SIST_INDEX_ID_LEN]);

void index_json(cJSON *document, const char doc_id[SIST_INDEX_ID_LEN]);

void delete_document(const char *document_id_str, void* data);

es_indexer_t *create_indexer(const char *url, const char *index);

void elastic_cleanup();
void finish_indexer(char *script, int async_script, char *index_id);

void elastic_init(int force_reset, const char* user_mappings, const char* user_settings);

cJSON *elastic_get_document(const char *id_str);

char *elastic_get_status();

es_version_t *elastic_get_version(const char *es_url);

void execute_update_script(const char *script, int async, const char index_id[SIST_INDEX_ID_LEN]);

#endif
