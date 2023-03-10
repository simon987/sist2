#include "walk.h"
#include "src/ctx.h"
#include "src/parsing/parse.h"

#include <ftw.h>

#define STR_STARTS_WITH(x, y) (strncmp(y, x, strlen(y) - 1) == 0)

__always_inline
parse_job_t *create_fs_parse_job(const char *filepath, const struct stat *info, int base) {
    int len = (int) strlen(filepath);
    parse_job_t *job = malloc(sizeof(parse_job_t));

    strcpy(job->filepath, filepath);
    job->base = base;
    char *p = strrchr(filepath + base, '.');
    if (p != NULL) {
        job->ext = (int) (p - filepath + 1);
    } else {
        job->ext = len;
    }

    job->vfile.st_size = info->st_size;
    job->vfile.st_mode = info->st_mode;
    job->vfile.mtime = (int) info->st_mtim.tv_sec;

    job->parent[0] = '\0';

    memcpy(job->vfile.filepath, job->filepath, sizeof(job->vfile.filepath));
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

    if (ftw->level > ScanCtx.depth) {
        if (typeflag == FTW_D) {
            return FTW_SKIP_SUBTREE;
        }
        return FTW_CONTINUE;
    }

    if (ScanCtx.exclude != NULL && EXCLUDED(filepath)) {
        LOG_DEBUGF("walk.c", "Excluded: %s", filepath)

        if (typeflag == FTW_F && S_ISREG(info->st_mode)) {
            pthread_mutex_lock(&ScanCtx.dbg_file_counts_mu);
            ScanCtx.dbg_excluded_files_count += 1;
            pthread_mutex_unlock(&ScanCtx.dbg_file_counts_mu);
        } else if (typeflag == FTW_D) {
            return FTW_SKIP_SUBTREE;
        }

        return FTW_CONTINUE;
    }

    if (typeflag == FTW_F && S_ISREG(info->st_mode)) {
        parse_job_t *job = create_fs_parse_job(filepath, info, ftw->base);

        tpool_work_arg_t arg = {
            .arg_size = sizeof(parse_job_t),
            .arg = job
        };
        tpool_add_work(ScanCtx.pool, parse, &arg);
    }

    return FTW_CONTINUE;
}

#define MAX_FILE_DESCRIPTORS 64

int walk_directory_tree(const char *dirpath) {
    return nftw(dirpath, handle_entry, MAX_FILE_DESCRIPTORS, FTW_PHYS | FTW_ACTIONRETVAL);
}

int iterate_file_list(void *input_file) {

    char buf[PATH_MAX];
    struct stat info;

    while (fgets(buf, sizeof(buf), input_file) != NULL) {

        // Remove trailing newline
        *(buf + strlen(buf) - 1) = '\0';

        int stat_ret = stat(buf, &info);

        if (stat_ret != 0) {
            LOG_ERRORF("walk.c", "Could not stat file %s (%s)", buf, strerror(errno));
            continue;
        }

        if (!S_ISREG(info.st_mode)) {
            LOG_ERRORF("walk.c", "Is not a regular file: %s", buf);
            continue;
        }

        char *absolute_path = canonicalize_file_name(buf);

        if (absolute_path == NULL) {
            LOG_FATALF("walk.c", "FIXME: Could not get absolute path of %s", buf);
        }

        if (ScanCtx.exclude != NULL && EXCLUDED(absolute_path)) {
            LOG_DEBUGF("walk.c", "Excluded: %s", absolute_path)

            if (S_ISREG(info.st_mode)) {
                pthread_mutex_lock(&ScanCtx.dbg_file_counts_mu);
                ScanCtx.dbg_excluded_files_count += 1;
                pthread_mutex_unlock(&ScanCtx.dbg_file_counts_mu);
            }

            continue;
        }

        if (!STR_STARTS_WITH(absolute_path, ScanCtx.index.desc.root)) {
            LOG_FATALF("walk.c", "File is not a children of root folder (%s): %s", ScanCtx.index.desc.root, buf);
        }

        int base = (int) (strrchr(buf, '/') - buf) + 1;

        parse_job_t *job = create_fs_parse_job(absolute_path, &info, base);
        free(absolute_path);

        tpool_work_arg_t arg = {
            .arg = job,
            .arg_size = sizeof(parse_job_t)
        };
        tpool_add_work(ScanCtx.pool, parse, &arg);
    }

    return 0;
}