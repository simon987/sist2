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
    // Filesystem reads are always rewindable
    job->vfile.read_rewindable = fs_read;
    job->vfile.reset = fs_reset;
    job->vfile.close = fs_close;
    job->vfile.fd = -1;
    job->vfile.is_fs_file = TRUE;
    job->vfile.has_checksum = FALSE;
    job->vfile.rewind_buffer_size = 0;
    job->vfile.rewind_buffer = NULL;
    job->vfile.calculate_checksum = ScanCtx.calculate_checksums;

    return job;
}

int sub_strings[30];
#define EXCLUDED(str) (pcre_exec(ScanCtx.exclude, ScanCtx.exclude_extra, str, strlen(str), 0, 0, sub_strings, sizeof(sub_strings)) >= 0)

int handle_entry(const char *filepath, const struct stat *info, int typeflag, struct FTW *ftw) {

    if (typeflag == FTW_F && S_ISREG(info->st_mode) && ftw->level <= ScanCtx.depth) {

        if (ScanCtx.exclude != NULL && EXCLUDED(filepath)) {
            LOG_DEBUGF("walk.c", "Excluded: %s", filepath)

            pthread_mutex_lock(&ScanCtx.dbg_file_counts_mu);
            ScanCtx.dbg_excluded_files_count += 1;
            pthread_mutex_unlock(&ScanCtx.dbg_file_counts_mu);
            return 0;
        }

        parse_job_t *job = create_fs_parse_job(filepath, info, ftw->base);
        tpool_add_work(ScanCtx.pool, parse, job);
    }

    return 0;
}

#define MAX_FILE_DESCRIPTORS 64

int walk_directory_tree(const char *dirpath) {
    return nftw(dirpath, handle_entry, MAX_FILE_DESCRIPTORS, FTW_PHYS | FTW_DEPTH);
}
