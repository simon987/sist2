#include "parse.h"

#include "src/sist.h"
#include "src/ctx.h"
#include "mime.h"
#include "src/io/serialize.h"
#include "src/parsing/sidecar.h"

#include <magic.h>


#define MIN_VIDEO_SIZE 1024 * 64
#define MIN_IMAGE_SIZE 1024 * 2

int fs_read(struct vfile *f, void *buf, size_t size) {

    if (f->fd == -1) {
        f->fd = open(f->filepath, O_RDONLY);
        if (f->fd == -1) {
            LOG_ERRORF(f->filepath, "open(): [%d] %s", errno, strerror(errno))
            return -1;
        }
    }

    return read(f->fd, buf, size);
}

#define CLOSE_FILE(f) if (f.close != NULL) {f.close(&f);};

void fs_close(struct vfile *f) {
    if (f->fd != -1) {
        close(f->fd);
    }
}

void fs_reset(struct vfile *f) {
    if (f->fd != -1) {
        lseek(f->fd, 0, SEEK_SET);
    }
}

#define IS_GIT_OBJ (strlen(doc.filepath + doc.base) == 38 && (strstr(doc.filepath, "objects") != NULL))

void parse(void *arg) {

    parse_job_t *job = arg;
    document_t doc;

    int inc_ts = incremental_get(ScanCtx.original_table, job->vfile.info.st_ino);
    if (inc_ts != 0 && inc_ts == job->vfile.info.st_mtim.tv_sec) {
        incremental_mark_file_for_copy(ScanCtx.copy_table, job->vfile.info.st_ino);
        return;
    }

    doc.filepath = job->filepath;
    doc.ext = (short) job->ext;
    doc.base = (short) job->base;
    doc.meta_head = NULL;
    doc.meta_tail = NULL;
    doc.mime = 0;
    doc.size = job->vfile.info.st_size;
    doc.ino = job->vfile.info.st_ino;
    doc.mtime = job->vfile.info.st_mtim.tv_sec;

    uuid_generate(doc.uuid);
    char *buf[MAGIC_BUF_SIZE];

    if (LogCtx.very_verbose) {
        char uuid_str[UUID_STR_LEN];
        uuid_unparse(doc.uuid, uuid_str);
        LOG_DEBUGF(job->filepath, "Starting parse job {%s}", uuid_str)
    }

    if (job->vfile.info.st_size == 0) {
        doc.mime = MIME_EMPTY;
    } else if (*(job->filepath + job->ext) != '\0' && (job->ext - job->base != 1)) {
        doc.mime = mime_get_mime_by_ext(ScanCtx.ext_table, job->filepath + job->ext);
    }

    int bytes_read = 0;

    if (doc.mime == 0 && !ScanCtx.fast) {
        if (IS_GIT_OBJ) {
            goto abort;
        }

        // Get mime type with libmagic
        if (!job->vfile.is_fs_file) {
            LOG_WARNING(job->filepath, "Guessing mime type with libmagic inside archive files is not currently supported");
            goto abort;
        }

        bytes_read = job->vfile.read(&job->vfile, buf, MAGIC_BUF_SIZE);
        if (bytes_read < 0) {

            if (job->vfile.is_fs_file) {
                LOG_ERRORF(job->filepath, "read(): [%d] %s", errno, strerror(errno))
            } else {
                LOG_ERRORF(job->filepath, "(virtual) read(): [%d] %s", bytes_read, archive_error_string(job->vfile.arc))
            }

            CLOSE_FILE(job->vfile)
            return;
        }

        magic_t magic = magic_open(MAGIC_MIME_TYPE);
        magic_load(magic, NULL);

        const char *magic_mime_str = magic_buffer(magic, buf, bytes_read);
        if (magic_mime_str != NULL) {
            doc.mime = mime_get_mime_by_string(ScanCtx.mime_table, magic_mime_str);

            LOG_DEBUGF(job->filepath, "libmagic: %s", magic_mime_str);

            if (doc.mime == 0) {
                LOG_WARNINGF(job->filepath, "Couldn't find mime %s", magic_mime_str);
            }
        }

        job->vfile.reset(&job->vfile);

        magic_close(magic);
    }

    int mmime = MAJOR_MIME(doc.mime);

    if (!(SHOULD_PARSE(doc.mime))) {

    } else if (IS_RAW(doc.mime)) {
        parse_raw(&ScanCtx.raw_ctx, &job->vfile, &doc);
    } else if ((mmime == MimeVideo && doc.size >= MIN_VIDEO_SIZE) ||
               (mmime == MimeImage && doc.size >= MIN_IMAGE_SIZE) || mmime == MimeAudio) {

        parse_media(&ScanCtx.media_ctx, &job->vfile, &doc);

    } else if (IS_PDF(doc.mime)) {
        parse_ebook(&ScanCtx.ebook_ctx, &job->vfile, mime_get_mime_text(doc.mime), &doc);

    } else if (mmime == MimeText && ScanCtx.text_ctx.content_size > 0) {
        if (IS_MARKUP(doc.mime)) {
            parse_markup(&ScanCtx.text_ctx, &job->vfile, &doc);
        } else {
            parse_text(&ScanCtx.text_ctx, &job->vfile, &doc);
        }

    } else if (IS_FONT(doc.mime)) {
        parse_font(&ScanCtx.font_ctx, &job->vfile, &doc);

    } else if (
            ScanCtx.arc_ctx.mode != ARC_MODE_SKIP && (
                    IS_ARC(doc.mime) ||
                    (IS_ARC_FILTER(doc.mime) && should_parse_filtered_file(doc.filepath, doc.ext))
            )) {
        parse_archive(&ScanCtx.arc_ctx, &job->vfile, &doc);
    } else if ((ScanCtx.ooxml_ctx.content_size > 0 || ScanCtx.media_ctx.tn_size > 0) && IS_DOC(doc.mime)) {
        parse_ooxml(&ScanCtx.ooxml_ctx, &job->vfile, &doc);
    } else if (is_cbr(&ScanCtx.comic_ctx, doc.mime) || is_cbz(&ScanCtx.comic_ctx, doc.mime)) {
        parse_comic(&ScanCtx.comic_ctx, &job->vfile, &doc);
    } else if (IS_MOBI(doc.mime)) {
        parse_mobi(&ScanCtx.mobi_ctx, &job->vfile, &doc);
    } else if (doc.mime == MIME_SIST2_SIDECAR) {
        parse_sidecar(&job->vfile, &doc);
        CLOSE_FILE(job->vfile)
        return;
    }

    abort:

    //Parent meta
    if (!uuid_is_null(job->parent)) {
        meta_line_t *meta_parent = malloc(sizeof(meta_line_t) + UUID_STR_LEN + 1);
        meta_parent->key = MetaParent;
        uuid_unparse(job->parent, meta_parent->str_val);
        APPEND_META((&doc), meta_parent)
    }

    write_document(&doc);

    CLOSE_FILE(job->vfile)
}

void cleanup_parse() {
    // noop
}
