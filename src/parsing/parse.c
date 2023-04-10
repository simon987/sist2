#include "parse.h"

#include "src/sist.h"
#include "src/ctx.h"
#include "mime.h"
#include "src/io/serialize.h"
#include "src/parsing/sidecar.h"
#include "src/parsing/fs_util.h"
#include "src/parsing/magic_util.h"
#include <pthread.h>


#define MIN_VIDEO_SIZE (1024 * 64)
#define MIN_IMAGE_SIZE (512)

#define MAGIC_BUF_SIZE (4096 * 6)

typedef enum {
    FILETYPE_DONT_PARSE,
    FILETYPE_RAW,
    FILETYPE_MEDIA,
    FILETYPE_EBOOK,
    FILETYPE_MARKUP,
    FILETYPE_TEXT,
    FILETYPE_FONT,
    FILETYPE_ARCHIVE,
    FILETYPE_OOXML,
    FILETYPE_COMIC,
    FILETYPE_MOBI,
    FILETYPE_SIST2_SIDECAR,
    FILETYPE_MSDOC,
    FILETYPE_JSON,
    FILETYPE_NDJSON,
} file_type_t;

file_type_t get_file_type(unsigned int mime, size_t size, const char *filepath) {

    int major_mime = MAJOR_MIME(mime);

    if (!(SHOULD_PARSE(mime))) {
        return FILETYPE_DONT_PARSE;
    } else if (IS_RAW(mime)) {
        return FILETYPE_RAW;
    } else if ((major_mime == MimeVideo && size >= MIN_VIDEO_SIZE) ||
               (major_mime == MimeImage && size >= MIN_IMAGE_SIZE) || major_mime == MimeAudio) {
        return FILETYPE_MEDIA;
    } else if (IS_PDF(mime)) {
        return FILETYPE_EBOOK;
    } else if (IS_MARKUP(mime)) {
        return FILETYPE_MARKUP;
    } else if (major_mime == MimeText) {
        return FILETYPE_TEXT;
    } else if (IS_FONT(mime)) {
        return FILETYPE_FONT;
    } else if (ScanCtx.arc_ctx.mode != ARC_MODE_SKIP && (
                    IS_ARC(mime) ||
                    (IS_ARC_FILTER(mime) && should_parse_filtered_file(filepath))
            )) {
        return FILETYPE_ARCHIVE;
    } else if ((ScanCtx.ooxml_ctx.content_size > 0 || ScanCtx.media_ctx.tn_size > 0) && IS_DOC(mime)) {
        return FILETYPE_OOXML;
    } else if (is_cbr(&ScanCtx.comic_ctx, mime) || is_cbz(&ScanCtx.comic_ctx, mime)) {
        return FILETYPE_COMIC;
    } else if (IS_MOBI(mime)) {
        return FILETYPE_MOBI;
    } else if (mime == MIME_SIST2_SIDECAR) {
        return FILETYPE_SIST2_SIDECAR;
    } else if (is_msdoc(&ScanCtx.msdoc_ctx, mime)) {
        return FILETYPE_MSDOC;
    } else if (is_json(&ScanCtx.json_ctx, mime)) {
        return FILETYPE_JSON;
    } else if (is_ndjson(&ScanCtx.json_ctx, mime)) {
        return FILETYPE_NDJSON;
    }
}

#define GET_MIME_ERROR_FATAL (-1)

int get_mime(parse_job_t *job) {

    char *extension = job->filepath + job->ext;

    int mime = 0;

    if (job->vfile.st_size == 0) {
        return MIME_EMPTY;
    }

    if (*extension != '\0' && (job->ext - job->base != 1)) {
        mime = (int) mime_get_mime_by_ext(extension);

        if (mime != 0) {
            return mime;
        }
    }

    if (ScanCtx.fast) {
        return 0;
    }

    // Get mime type with libmagic
    if (job->vfile.read_rewindable == NULL) {
        LOG_WARNING(job->filepath,
                    "File does not support rewindable reads, cannot guess Media type");
        return 0;
    }

    char *buf[MAGIC_BUF_SIZE];
    int bytes_read = job->vfile.read_rewindable(&job->vfile, buf, MAGIC_BUF_SIZE);
    if (bytes_read < 0) {
        if (job->vfile.is_fs_file) {
            LOG_ERRORF(job->filepath, "read(): [%d] %s", errno, strerror(errno));
        } else {
            LOG_ERRORF(job->filepath, "(virtual) read(): [%d] %s", bytes_read, archive_error_string(job->vfile.arc));
        }

        return GET_MIME_ERROR_FATAL;
    }

    char *magic_mime_str = magic_buffer_embedded(buf, bytes_read);

    if (magic_mime_str != NULL) {
        mime = (int) mime_get_mime_by_string(magic_mime_str);

        if (mime == 0) {
            LOG_WARNINGF(job->filepath, "Couldn't find mime %s", magic_mime_str);
            free(magic_mime_str);
            return 0;
        }
        free(magic_mime_str);
    }

    if (job->vfile.reset != NULL) {
        job->vfile.reset(&job->vfile);
    }

    return mime;
}

void parse(parse_job_t *job) {

    if (job->vfile.is_fs_file) {
        job->vfile.read = fs_read;
        job->vfile.read_rewindable = fs_read;
        job->vfile.reset = fs_reset;
        job->vfile.close = fs_close;
        job->vfile.calculate_checksum = ScanCtx.calculate_checksums;
    }

    document_t *doc = malloc(sizeof(document_t));

    strcpy(doc->filepath, job->filepath);
    doc->ext = job->ext;
    doc->base = job->base;
    doc->meta_head = NULL;
    doc->meta_tail = NULL;
    doc->size = job->vfile.st_size;
    doc->mtime = job->vfile.mtime;
    doc->mime = get_mime(job);
    generate_doc_id(doc->filepath + ScanCtx.index.desc.root_len, doc->doc_id);

    if (doc->mime == GET_MIME_ERROR_FATAL) {

        CLOSE_FILE(job->vfile)
        free(doc);
        return;
    }

    if (database_mark_document(ProcData.index_db, doc->doc_id, doc->mtime)) {

        CLOSE_FILE(job->vfile)
        free(doc);
        return;
    }

    if (LogCtx.very_verbose) {
        LOG_DEBUGF(job->filepath, "Starting parse job {%s}", doc->doc_id);
    }

    switch (get_file_type(doc->mime, doc->size, doc->filepath)) {
        case FILETYPE_RAW:
            parse_raw(&ScanCtx.raw_ctx, &job->vfile, doc);
            break;
        case FILETYPE_MEDIA:
            parse_media(&ScanCtx.media_ctx, &job->vfile, doc, mime_get_mime_text(doc->mime));
            break;
        case FILETYPE_EBOOK:
            parse_ebook(&ScanCtx.ebook_ctx, &job->vfile, mime_get_mime_text(doc->mime), doc);
            break;
        case FILETYPE_MARKUP:
            parse_markup(&ScanCtx.text_ctx, &job->vfile, doc);
            break;
        case FILETYPE_TEXT:
            parse_text(&ScanCtx.text_ctx, &job->vfile, doc);
            break;
        case FILETYPE_FONT:
            parse_font(&ScanCtx.font_ctx, &job->vfile, doc);
            break;
        case FILETYPE_ARCHIVE:
            parse_archive(&ScanCtx.arc_ctx, &job->vfile, doc, ScanCtx.exclude, ScanCtx.exclude_extra);
            break;
        case FILETYPE_OOXML:
            parse_ooxml(&ScanCtx.ooxml_ctx, &job->vfile, doc);
            break;
        case FILETYPE_COMIC:
            parse_comic(&ScanCtx.comic_ctx, &job->vfile, doc);
            break;
        case FILETYPE_MOBI:
            parse_mobi(&ScanCtx.mobi_ctx, &job->vfile, doc);
            break;
        case FILETYPE_SIST2_SIDECAR:
            parse_sidecar(&job->vfile, doc);
            CLOSE_FILE(job->vfile)
            free(doc);
            return;
        case FILETYPE_MSDOC:
            parse_msdoc(&ScanCtx.msdoc_ctx, &job->vfile, doc);
            break;
        case FILETYPE_JSON:
            parse_json(&ScanCtx.json_ctx, &job->vfile, doc);
            break;
        case FILETYPE_NDJSON:
            parse_ndjson(&ScanCtx.json_ctx, &job->vfile, doc);
            break;
        case FILETYPE_DONT_PARSE:
        default:
            break;
    }

    //Parent meta
    if (job->parent[0] != '\0') {
        meta_line_t *meta_parent = malloc(sizeof(meta_line_t) + SIST_INDEX_ID_LEN);
        meta_parent->key = MetaParent;
        strcpy(meta_parent->str_val, job->parent);
        APPEND_META((doc), meta_parent);
    }

    CLOSE_FILE(job->vfile)

    if (job->vfile.has_checksum) {
        char sha1_digest_str[SHA1_STR_LENGTH];
        buf2hex((unsigned char *) job->vfile.sha1_digest, SHA1_DIGEST_LENGTH, (char *) sha1_digest_str);
        APPEND_STR_META(doc, MetaChecksum, (const char *) sha1_digest_str);
    }

    write_document(doc);
}
