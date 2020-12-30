#include "walk.h"
#include "src/ctx.h"
#include "src/parsing/parse.h"

#include <ftw.h>

__always_inline
parse_job_t *create_fs_parse_job(const char *filepath, const struct stat *info, int base) {
    int len = (int) strlen(filepath);
    parse_job_t *job = malloc(sizeof(parse_job_t) + len);

    strcpy(job->filepath, filepath);
    job->base = base;
    char *p = strrchr(filepath + base, '.');
    if (p != NULL) {
        job->ext = (int) (p - filepath + 1);
    } else {
        job->ext = len;
    }

    job->vfile.info = *info;

    memset(job->parent, 0, MD5_DIGEST_LENGTH);

    job->vfile.filepath = job->filepath;
    job->vfile.read = fs_read;
    job->vfile.reset = fs_reset;
    job->vfile.close = fs_close;
    job->vfile.fd = -1;
    job->vfile.is_fs_file = TRUE;

    return job;
}

int sub_strings[30];
#define EXCLUDED(str) (pcre_exec(ScanCtx.exclude, ScanCtx.exclude_extra, filepath, strlen(filepath), 0, 0, sub_strings, sizeof(sub_strings)) >= 0)

int handle_entry(const char *filepath, const struct stat *info, int typeflag, struct FTW *ftw) {

    if (typeflag == FTW_F && S_ISREG(info->st_mode) && ftw->level <= ScanCtx.depth) {

        if (ScanCtx.exclude != NULL && EXCLUDED(filepath)) {
            LOG_DEBUGF("walk.c", "Excluded: %s", filepath)
            return 0;
        }

        parse_job_t *job = create_fs_parse_job(filepath, info, ftw->base);
        tpool_add_work(ScanCtx.pool, parse, job);
    }

    return 0;
}

int walk_directory_tree(const char *dirpath) {
    return nftw(dirpath, handle_entry, 15, FTW_PHYS);
}
