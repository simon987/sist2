#include "sidecar.h"

#include "src/ctx.h"

void parse_sidecar(vfile_t *vfile, document_t *doc) {

    LOG_DEBUGF("sidecar.c", "Parsing sidecar file %s", vfile->filepath)

    size_t size;
    char *buf = read_all(vfile, &size);
    if (buf == NULL) {
        LOG_ERRORF("sidecar.c", "Read error for %s", vfile->filepath)
        return;
    }

    buf = realloc(buf, size + 1);
    *(buf + size) = '\0';

    cJSON *json = cJSON_Parse(buf);
    if (json == NULL) {
        LOG_ERRORF("sidecar.c", "Could not parse JSON sidecar %s", vfile->filepath)
        return;
    }
    char *json_str = cJSON_PrintUnformatted(json);

    unsigned char path_md5[MD5_DIGEST_LENGTH];
    MD5((unsigned char *) vfile->filepath + ScanCtx.index.desc.root_len, doc->ext - 1 - ScanCtx.index.desc.root_len,
        path_md5);

    store_write(ScanCtx.index.meta_store, (char *) path_md5, sizeof(path_md5), json_str, strlen(json_str) + 1);

    cJSON_Delete(json);
    free(json_str);
    free(buf);
}
