#include <src/ctx.h>
#include "src/sist.h"
#include "src/ctx.h"

__thread magic_t Magic = NULL;

int fs_read(struct vfile *f, void *buf, size_t size) {

    if (f->fd == -1) {
        f->fd = open(f->filepath, O_RDONLY);
        if (f->fd == -1) {
            perror("open");
            printf("%s\n", f->filepath);
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

void *read_all(parse_job_t *job, const char *buf, int bytes_read) {

    void *full_buf;

    if (job->info.st_size <= bytes_read) {
        full_buf = malloc(job->info.st_size);
        memcpy(full_buf, buf, job->info.st_size);
    } else {
        full_buf = malloc(job->info.st_size);
        memcpy(full_buf, buf, bytes_read);

        int ret = job->vfile.read(&job->vfile, full_buf + bytes_read, job->info.st_size - bytes_read);
        if (ret == -1) {
            perror("read");
            return NULL;
        }
    }

    return full_buf;
}

void parse(void *arg) {

    parse_job_t *job = arg;
    document_t doc;

    int inc_ts = incremental_get(ScanCtx.original_table, job->info.st_ino);
    if (inc_ts != 0 && inc_ts == job->info.st_mtim.tv_sec) {
        incremental_mark_file_for_copy(ScanCtx.copy_table, job->info.st_ino);
        return;
    }

    if (Magic == NULL) {
        Magic = magic_open(MAGIC_MIME_TYPE);
    }

    doc.filepath = job->filepath;
    doc.ext = (short) job->ext;
    doc.base = (short) job->base;
    doc.meta_head = NULL;
    doc.meta_tail = NULL;
    doc.mime = 0;
    doc.size = job->info.st_size;
    doc.ino = job->info.st_ino;
    doc.mtime = job->info.st_mtim.tv_sec;

    uuid_generate(doc.uuid);
    char *buf[PARSE_BUF_SIZE];

    if (job->info.st_size == 0) {
        doc.mime = MIME_EMPTY;
    } else if (*(job->filepath + job->ext) != '\0' && (job->ext - job->base != 1)) {
        doc.mime = mime_get_mime_by_ext(ScanCtx.ext_table, job->filepath + job->ext);
    }

    int bytes_read = 0;

    if (doc.mime == 0) {
        // Get mime type with libmagic
        bytes_read = job->vfile.read(&job->vfile, buf, PARSE_BUF_SIZE);
        if (bytes_read == -1) {
            CLOSE_FILE(job->vfile)
            return;
        }

        const char *magic_mime_str = magic_buffer(Magic, buf, bytes_read);
        if (magic_mime_str != NULL) {
            doc.mime = mime_get_mime_by_string(ScanCtx.mime_table, magic_mime_str);
            if (doc.mime == 0) {
                fprintf(stderr, "Couldn't find mime %s, %s\n", magic_mime_str, job->filepath + job->base);
            }
        }
    }

    int mmime = MAJOR_MIME(doc.mime);

    if (!(SHOULD_PARSE(doc.mime))) {

    } else if ((mmime == MimeVideo && doc.size >= MIN_VIDEO_SIZE) ||
               (mmime == MimeImage && doc.size >= MIN_IMAGE_SIZE) || mmime == MimeAudio) {

        if (job->vfile.is_fs_file) {
            parse_media_filename(job->filepath, &doc);
        } else {
            parse_media_vfile(&job->vfile, &doc);
        }

    } else if (IS_PDF(doc.mime)) {
        void *pdf_buf = read_all(job, (char *) buf, bytes_read);
        parse_pdf(pdf_buf, doc.size, &doc);

        if (pdf_buf != buf && pdf_buf != NULL) {
            free(pdf_buf);
        }

    } else if (mmime == MimeText && ScanCtx.content_size > 0) {
        parse_text(bytes_read, &job->vfile, (char *) buf, &doc);

    } else if (IS_FONT(doc.mime)) {
        void *font_buf = read_all(job, (char *) buf, bytes_read);
        parse_font(font_buf, doc.size, &doc);

        if (font_buf != buf && font_buf != NULL) {
            free(font_buf);
        }
    } else if (
            ScanCtx.archive_mode != ARC_MODE_SKIP && (
                    IS_ARC(doc.mime) ||
                    (IS_ARC_FILTER(doc.mime) && should_parse_filtered_file(doc.filepath, doc.ext))
            )) {
        parse_archive(&job->vfile, &doc);
    } else if (ScanCtx.content_size > 0 && IS_DOC(doc.mime)) {
        void *doc_buf = read_all(job, (char *) buf, bytes_read);
        parse_doc(doc_buf, doc.size, &doc);

        if (doc_buf != buf && doc_buf != NULL) {
            free(doc_buf);
        }
    }

    //Parent meta
    if (!uuid_is_null(job->parent)) {
        char tmp[UUID_STR_LEN];
        uuid_unparse(job->parent, tmp);

        meta_line_t *meta_parent = malloc(sizeof(meta_line_t) + UUID_STR_LEN + 1);
        meta_parent->key = MetaParent;
        strcpy(meta_parent->strval, tmp);
        APPEND_META((&doc), meta_parent)
    }

    write_document(&doc);

    CLOSE_FILE(job->vfile)
}
