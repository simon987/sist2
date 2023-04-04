#include "sidecar.h"

#include "src/ctx.h"

void parse_sidecar(vfile_t *vfile, document_t *doc) {

    LOG_DEBUGF("sidecar.c", "Parsing sidecar file %s", vfile->filepath);

    size_t size;
    char *buf = read_all(vfile, &size);
    if (buf == NULL) {
        LOG_ERRORF("sidecar.c", "Read error for %s", vfile->filepath);
        return;
    }

    buf = realloc(buf, size + 1);
    *(buf + size) = '\0';

    cJSON *json = cJSON_Parse(buf);
    if (json == NULL) {
        LOG_ERRORF("sidecar.c", "Could not parse JSON sidecar %s", vfile->filepath);
        return;
    }
    char *json_str = cJSON_PrintUnformatted(json);

    char assoc_doc_id[SIST_DOC_ID_LEN];

    char rel_path[PATH_MAX];
    size_t rel_path_len = doc->ext - 1 - ScanCtx.index.desc.root_len;
    memcpy(rel_path, vfile->filepath + ScanCtx.index.desc.root_len, rel_path_len);
    *(rel_path + rel_path_len) = '\0';

    generate_doc_id(rel_path, assoc_doc_id);

    database_write_document_sidecar(ProcData.index_db, assoc_doc_id, json_str);

    cJSON_Delete(json);
    free(json_str);
    free(buf);
}