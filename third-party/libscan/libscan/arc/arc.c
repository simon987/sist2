#include "arc.h"

#include "../scan.h"
#include "../util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>



int should_parse_filtered_file(const char *filepath, int ext) {
    char tmp[PATH_MAX * 2];

    if (ext == 0) {
        return FALSE;
    }

    memcpy(tmp, filepath, ext - 1);
    *(tmp + ext - 1) = '\0';

    char *idx = strrchr(tmp, '.');

    if (idx == NULL) {
        return FALSE;
    }

    if (strcmp(idx, ".tar") == 0) {
        return TRUE;
    }

    return FALSE;
}

int arc_read(struct vfile *f, void *buf, size_t size) {
    return archive_read_data(f->arc, buf, size);
}

typedef struct arc_data {
    vfile_t *f;
    char buf[ARC_BUF_SIZE];
} arc_data_f;

int vfile_open_callback(struct archive *a, void *user_data) {
    arc_data_f *data = user_data;

    if (data->f->is_fs_file && data->f->fd == -1) {
        data->f->fd = open(data->f->filepath, O_RDONLY);
    }

    return ARCHIVE_OK;
}

long vfile_read_callback(struct archive *a, void *user_data, const void **buf) {
    arc_data_f *data = user_data;

    *buf = data->buf;
    return data->f->read(data->f, data->buf, ARC_BUF_SIZE);
}

int vfile_close_callback(struct archive *a, void *user_data) {
    arc_data_f *data = user_data;

    if (data->f->close != NULL) {
        data->f->close(data->f);
    }

    return ARCHIVE_OK;
}

scan_code_t parse_archive(scan_arc_ctx_t *ctx, vfile_t *f, document_t *doc) {

    struct archive *a;
    struct archive_entry *entry;


    arc_data_f data;
    data.f = f;

    int ret = 0;
    if (data.f->is_fs_file) {

        a = archive_read_new();
        archive_read_support_filter_all(a);
        archive_read_support_format_all(a);

        ret = archive_read_open_filename(a, doc->filepath, ARC_BUF_SIZE);
    } else if (ctx->mode == ARC_MODE_RECURSE) {

        a = archive_read_new();
        archive_read_support_filter_all(a);
        archive_read_support_format_all(a);

        ret = archive_read_open(
                a, &data,
                vfile_open_callback,
                vfile_read_callback,
                vfile_close_callback
        );
    } else {
        return SCAN_OK;
    }

    if (ret != ARCHIVE_OK) {
        //TODO: log
//        LOG_ERRORF(doc->filepath, "(arc.c) [%d] %s", ret, archive_error_string(a))
        archive_read_free(a);
        return SCAN_ERR_READ;
    }

    if (ctx->mode == ARC_MODE_LIST) {

        dyn_buffer_t buf = dyn_buffer_create();

        while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
            if (S_ISREG(archive_entry_stat(entry)->st_mode)) {

                char *path = (char *) archive_entry_pathname(entry);

                dyn_buffer_append_string(&buf, path);
                dyn_buffer_write_char(&buf, '\n');
            }
        }
        dyn_buffer_write_char(&buf, '\0');

        meta_line_t *meta_list = malloc(sizeof(meta_line_t) + buf.cur);
        meta_list->key = MetaContent;
        strcpy(meta_list->str_val, buf.buf);
        APPEND_META(doc, meta_list);
        dyn_buffer_destroy(&buf);

    } else {

        parse_job_t *sub_job = malloc(sizeof(parse_job_t) + PATH_MAX * 2);

        sub_job->vfile.close = NULL;
        sub_job->vfile.read = arc_read;
        sub_job->vfile.arc = a;
        sub_job->vfile.filepath = sub_job->filepath;
        sub_job->vfile.is_fs_file = FALSE;
        memcpy(sub_job->parent, doc->uuid, sizeof(uuid_t));

        while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
            sub_job->info = *archive_entry_stat(entry);
            if (S_ISREG(sub_job->info.st_mode)) {
                sprintf(sub_job->filepath, "%s#/%s", f->filepath, archive_entry_pathname(entry));
                sub_job->base = (int) (strrchr(sub_job->filepath, '/') - sub_job->filepath) + 1;

                char *p = strrchr(sub_job->filepath, '.');
                if (p != NULL) {
                    sub_job->ext = (int) (p - sub_job->filepath + 1);
                } else {
                    sub_job->ext = (int) strlen(sub_job->filepath);
                }

                //TODO:
//                parse(sub_job);
            }
        }

        free(sub_job);
    }

    archive_read_free(a);
    return SCAN_OK;
}
