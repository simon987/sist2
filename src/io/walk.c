#include "walk.h"
#include "src/ctx.h"

parse_job_t *create_parse_job(const char *filepath, const struct stat *info, int base) {
    int len = (int) strlen(filepath);

    parse_job_t *job = malloc(sizeof(parse_job_t) + len);

    memcpy(&(job->filepath), filepath, len + 1);
    job->base = base;
    char *p = strrchr(filepath + base, '.');
    if (p != NULL) {
        job->ext = (int)(p - filepath + 1);
    } else {
        job->ext = len;
    }

    memcpy(&(job->info), info, sizeof(struct stat));

    return job;
}

int handle_entry(const char *filepath, const struct stat *info, int typeflag, struct FTW *ftw) {
    if (typeflag == FTW_F && S_ISREG(info->st_mode)) {
        parse_job_t *job = create_parse_job(filepath, info, ftw->base);
        tpool_add_work(ScanCtx.pool, parse, job);
    }

    return 0;
}

int walk_directory_tree(const char *dirpath) {
    return nftw(dirpath, handle_entry, 15, FTW_PHYS);
}
