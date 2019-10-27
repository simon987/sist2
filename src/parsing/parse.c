#include "src/sist.h"
#include "src/ctx.h"

__thread magic_t Magic;

void *read_all(parse_job_t *job, const char *buf, int bytes_read, int *fd) {

    void *full_buf;

    if (job->info.st_size <= bytes_read) {
        full_buf = malloc(job->info.st_size);
        memcpy(full_buf, buf, job->info.st_size);
    } else {
        if (*fd == -1) {
            *fd = open(job->filepath, O_RDONLY);
            if (*fd == -1) {
                perror("open");
                printf("%s\n", job->filepath);
                free(job);
                return NULL;
            }
        }
        full_buf = malloc(job->info.st_size);
        memcpy(full_buf, buf, bytes_read);
        int ret = read(*fd, full_buf + bytes_read, job->info.st_size - bytes_read);
        if (ret == -1) {
            perror("read");
        }
    }

    return full_buf;
}

void parse(void *arg) {

    parse_job_t *job = arg;
    document_t doc;

    if (incremental_get(ScanCtx.original_table, job->info.st_ino) == job->info.st_mtim.tv_sec) {
        incremental_mark_file_for_copy(ScanCtx.copy_table, job->info.st_ino);
        free(job);
        return;
    }

    if (Magic == NULL) {
        Magic = magic_open(MAGIC_MIME_TYPE);
        magic_load(Magic, NULL);
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
    } else if (*(job->filepath + job->ext) != '\0') {
        doc.mime = mime_get_mime_by_ext(ScanCtx.ext_table, job->filepath + job->ext);
    }

    int fd = -1;
    int bytes_read = 0;

    if (doc.mime == 0) {
        // Get mime type with libmagic
        fd = open(job->filepath, O_RDONLY);
        if (fd == -1) {
            perror("open");
            free(job);
            return;
        }

        bytes_read = read(fd, buf, PARSE_BUF_SIZE);

        const char *magic_mime_str = magic_buffer(Magic, buf, bytes_read);
        if (magic_mime_str != NULL) {
            doc.mime = mime_get_mime_by_string(ScanCtx.mime_table, magic_mime_str);
            if (doc.mime == 0) {
                fprintf(stderr, "Couldn't find mime %s, %s!\n", magic_mime_str, job->filepath + job->base);
            }
        }
    }

    int mmime = MAJOR_MIME(doc.mime);

    if (!(SHOULD_PARSE(doc.mime))) {

    } else if ((mmime == MimeVideo && doc.size >= MIN_VIDEO_SIZE) || mmime == MimeAudio || mmime == MimeImage) {
        parse_media(job->filepath, &doc);

    } else if (IS_PDF(doc.mime)) {
        void *pdf_buf = read_all(job, (char *) buf, bytes_read, &fd);
        parse_pdf(pdf_buf, doc.size, &doc);

        if (pdf_buf != buf) {
            free(pdf_buf);
        }

    } else if (mmime == MimeText && ScanCtx.content_size > 0) {
        parse_text(bytes_read, &fd, (char *) buf, &doc);

    } else if (IS_FONT(doc.mime)) {
        void *font_buf = read_all(job, (char *) buf, bytes_read, &fd);
        parse_font(font_buf, doc.size, &doc);

        if (font_buf != buf) {
            free(font_buf);
        }
    }

    write_document(&doc);

    if (fd != -1) {
        close(fd);
    }

    free(job);
}
