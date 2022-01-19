#include "src/ctx.h"
#include "serialize.h"
#include "src/parsing/parse.h"
#include "src/parsing/mime.h"

#include <zstd.h>

char *get_meta_key_text(enum metakey meta_key) {

    switch (meta_key) {
        case MetaContent:
            return "content";
        case MetaWidth:
            return "width";
        case MetaHeight:
            return "height";
        case MetaMediaDuration:
            return "duration";
        case MetaMediaAudioCodec:
            return "audioc";
        case MetaMediaVideoCodec:
            return "videoc";
        case MetaMediaBitrate:
            return "bitrate";
        case MetaArtist:
            return "artist";
        case MetaAlbum:
            return "album";
        case MetaAlbumArtist:
            return "album_artist";
        case MetaGenre:
            return "genre";
        case MetaTitle:
            return "title";
        case MetaFontName:
            return "font_name";
        case MetaParent:
            return "parent";
        case MetaExifMake:
            return "exif_make";
        case MetaExifDescription:
            return "exif_description";
        case MetaExifSoftware:
            return "exif_software";
        case MetaExifExposureTime:
            return "exif_exposure_time";
        case MetaExifFNumber:
            return "exif_fnumber";
        case MetaExifFocalLength:
            return "exif_focal_length";
        case MetaExifUserComment:
            return "exif_user_comment";
        case MetaExifIsoSpeedRatings:
            return "exif_iso_speed_ratings";
        case MetaExifModel:
            return "exif_model";
        case MetaExifDateTime:
            return "exif_datetime";
        case MetaAuthor:
            return "author";
        case MetaModifiedBy:
            return "modified_by";
        case MetaThumbnail:
            return "thumbnail";
        case MetaPages:
            return "pages";
        case MetaExifGpsLongitudeRef:
            return "exif_gps_longitude_ref";
        case MetaExifGpsLongitudeDMS:
            return "exif_gps_longitude_dms";
        case MetaExifGpsLongitudeDec:
            return "exif_gps_longitude_dec";
        case MetaExifGpsLatitudeRef:
            return "exif_gps_latitude_ref";
        case MetaExifGpsLatitudeDMS:
            return "exif_gps_latitude_dms";
        case MetaExifGpsLatitudeDec:
            return "exif_gps_latitude_dec";
        case MetaChecksum:
            return "checksum";
        default:
        LOG_FATALF("serialize.c", "FIXME: Unknown meta key: %d", meta_key)
    }
}

char *build_json_string(document_t *doc) {
    cJSON *json = cJSON_CreateObject();
    int buffer_size_guess = 8192;

    const char *mime_text = mime_get_mime_text(doc->mime);
    if (mime_text == NULL) {
        cJSON_AddNullToObject(json, "mime");
    } else {
        cJSON_AddStringToObject(json, "mime", mime_text);
    }
    cJSON_AddNumberToObject(json, "size", (double) doc->size);
    cJSON_AddNumberToObject(json, "mtime", doc->mtime);

    // Ignore root directory in the file path
    doc->ext = (short) (doc->ext - ScanCtx.index.desc.root_len);
    doc->base = (short) (doc->base - ScanCtx.index.desc.root_len);
    char *filepath = doc->filepath + ScanCtx.index.desc.root_len;

    cJSON_AddStringToObject(json, "extension", filepath + doc->ext);

    // Remove extension
    if (*(filepath + doc->ext - 1) == '.') {
        *(filepath + doc->ext - 1) = '\0';
    } else {
        *(filepath + doc->ext) = '\0';
    }

    char filepath_escaped[PATH_MAX * 3];
    str_escape(filepath_escaped, filepath + doc->base);

    cJSON_AddStringToObject(json, "name", filepath_escaped);

    if (doc->base > 0) {
        *(filepath + doc->base - 1) = '\0';

        str_escape(filepath_escaped, filepath);
        cJSON_AddStringToObject(json, "path", filepath_escaped);
    } else {
        cJSON_AddStringToObject(json, "path", "");
    }

    char md5_str[MD5_STR_LENGTH];
    buf2hex(doc->path_md5, MD5_DIGEST_LENGTH, md5_str);
    cJSON_AddStringToObject(json, "_id", md5_str);

    // Metadata
    meta_line_t *meta = doc->meta_head;
    while (meta != NULL) {

        switch (meta->key) {
            case MetaPages:
            case MetaWidth:
            case MetaHeight:
            case MetaMediaDuration:
            case MetaMediaBitrate: {
                cJSON_AddNumberToObject(json, get_meta_key_text(meta->key), (double) meta->long_val);
                buffer_size_guess += 20;
                break;
            }
            case MetaMediaAudioCodec:
            case MetaMediaVideoCodec:
            case MetaContent:
            case MetaArtist:
            case MetaAlbum:
            case MetaAlbumArtist:
            case MetaGenre:
            case MetaFontName:
            case MetaParent:
            case MetaExifMake:
            case MetaExifDescription:
            case MetaExifSoftware:
            case MetaExifExposureTime:
            case MetaExifFNumber:
            case MetaExifFocalLength:
            case MetaExifUserComment:
            case MetaExifIsoSpeedRatings:
            case MetaExifDateTime:
            case MetaExifModel:
            case MetaAuthor:
            case MetaModifiedBy:
            case MetaThumbnail:
            case MetaExifGpsLongitudeDMS:
            case MetaExifGpsLongitudeDec:
            case MetaExifGpsLongitudeRef:
            case MetaExifGpsLatitudeDMS:
            case MetaExifGpsLatitudeDec:
            case MetaExifGpsLatitudeRef:
            case MetaChecksum:
            case MetaTitle: {
                cJSON_AddStringToObject(json, get_meta_key_text(meta->key), meta->str_val);
                buffer_size_guess += (int) strlen(meta->str_val);
                break;
            }
            default:
            LOG_FATALF("serialize.c", "Invalid meta key: %x %s", meta->key, get_meta_key_text(meta->key))
        }

        meta_line_t *tmp = meta;
        meta = meta->next;
        free(tmp);
    }

    char *json_str = cJSON_PrintBuffered(json, buffer_size_guess, FALSE);
    cJSON_Delete(json);

    return json_str;
}

static struct {
    FILE *out_file;
    size_t buf_out_size;

    void *buf_out;

    ZSTD_CCtx *cctx;
} WriterCtx = {
        .out_file =  NULL
};

#define ZSTD_COMPRESSION_LEVEL 10

void initialize_writer_ctx(const char *file_path) {
    WriterCtx.out_file = fopen(file_path, "wb");

    WriterCtx.buf_out_size = ZSTD_CStreamOutSize();
    WriterCtx.buf_out = malloc(WriterCtx.buf_out_size);

    WriterCtx.cctx = ZSTD_createCCtx();

    ZSTD_CCtx_setParameter(WriterCtx.cctx, ZSTD_c_compressionLevel, ZSTD_COMPRESSION_LEVEL);
    ZSTD_CCtx_setParameter(WriterCtx.cctx, ZSTD_c_checksumFlag, FALSE);

    LOG_DEBUGF("serialize.c", "Open index file for writing %s", file_path)
}

void zstd_write_string(const char *string, const size_t len) {
    ZSTD_inBuffer input = {string, len, 0};

    do {
        ZSTD_outBuffer output = {WriterCtx.buf_out, WriterCtx.buf_out_size, 0};
        ZSTD_compressStream2(WriterCtx.cctx, &output, &input, ZSTD_e_continue);

        if (output.pos > 0) {
            ScanCtx.stat_index_size += fwrite(WriterCtx.buf_out, 1, output.pos, WriterCtx.out_file);
        }
    } while (input.pos != input.size);
}

void write_document_func(void *arg) {

    if (WriterCtx.out_file == NULL) {
        char dstfile[PATH_MAX];
        snprintf(dstfile, PATH_MAX, "%s_index_main.ndjson.zst", ScanCtx.index.path);
        initialize_writer_ctx(dstfile);
    }

    document_t *doc = arg;

    char *json_str = build_json_string(doc);
    const size_t json_str_len = strlen(json_str);

    json_str = realloc(json_str, json_str_len + 1);
    *(json_str + json_str_len) = '\n';

    zstd_write_string(json_str, json_str_len + 1);

    free(json_str);
    free(doc->filepath);
}

void zstd_close() {
    if (WriterCtx.out_file == NULL) {
        LOG_DEBUG("serialize.c", "No zstd stream to close, skipping cleanup")
        return;
    }

    size_t remaining;
    do {
        ZSTD_outBuffer output = {WriterCtx.buf_out, WriterCtx.buf_out_size, 0};
        remaining = ZSTD_endStream(WriterCtx.cctx, &output);

        if (output.pos > 0) {
            ScanCtx.stat_index_size += fwrite(WriterCtx.buf_out, 1, output.pos, WriterCtx.out_file);
        }
    } while (remaining != 0);

    ZSTD_freeCCtx(WriterCtx.cctx);
    free(WriterCtx.buf_out);
    fclose(WriterCtx.out_file);

    LOG_DEBUG("serialize.c", "End zstd stream & close index file")
}

void writer_cleanup() {
    zstd_close();
    WriterCtx.out_file = NULL;
}

void write_index_descriptor(char *path, index_descriptor_t *desc) {
    cJSON *json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "id", desc->id);
    cJSON_AddStringToObject(json, "version", desc->version);
    cJSON_AddStringToObject(json, "root", desc->root);
    cJSON_AddStringToObject(json, "name", desc->name);
    cJSON_AddStringToObject(json, "type", desc->type);
    cJSON_AddStringToObject(json, "rewrite_url", desc->rewrite_url);
    cJSON_AddNumberToObject(json, "timestamp", (double) desc->timestamp);

    int fd = open(path, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
    if (fd < 0) {
        LOG_FATALF("serialize.c", "Could not open index descriptor: %s", strerror(errno));
    }
    char *str = cJSON_Print(json);
    size_t ret = write(fd, str, strlen(str));
    if (ret == -1) {
        LOG_FATALF("serialize.c", "Could not write index descriptor: %s", strerror(errno));
    }
    free(str);
    close(fd);

    cJSON_Delete(json);
}

index_descriptor_t read_index_descriptor(char *path) {

    struct stat info;
    stat(path, &info);
    int fd = open(path, O_RDONLY);

    if (fd == -1) {
        LOG_FATALF("serialize.c", "Invalid/corrupt index (Could not find descriptor): %s: %s\n", path, strerror(errno))
    }

    char *buf = malloc(info.st_size + 1);
    size_t ret = read(fd, buf, info.st_size);
    if (ret == -1) {
        LOG_FATALF("serialize.c", "Could not read index descriptor: %s", strerror(errno));
    }
    *(buf + info.st_size) = '\0';
    close(fd);

    cJSON *json = cJSON_Parse(buf);

    index_descriptor_t descriptor;
    descriptor.timestamp = (long) cJSON_GetObjectItem(json, "timestamp")->valuedouble;
    strcpy(descriptor.root, cJSON_GetObjectItem(json, "root")->valuestring);
    strcpy(descriptor.name, cJSON_GetObjectItem(json, "name")->valuestring);
    strcpy(descriptor.rewrite_url, cJSON_GetObjectItem(json, "rewrite_url")->valuestring);
    descriptor.root_len = (short) strlen(descriptor.root);
    strcpy(descriptor.version, cJSON_GetObjectItem(json, "version")->valuestring);
    strcpy(descriptor.id, cJSON_GetObjectItem(json, "id")->valuestring);
    if (cJSON_GetObjectItem(json, "type") == NULL) {
        strcpy(descriptor.type, INDEX_TYPE_NDJSON);
    } else {
        strcpy(descriptor.type, cJSON_GetObjectItem(json, "type")->valuestring);
    }

    cJSON_Delete(json);
    free(buf);

    return descriptor;
}


void write_document(document_t *doc) {
    tpool_add_work(ScanCtx.writer_pool, write_document_func, doc);
}

void thread_cleanup() {
    cleanup_parse();
    cleanup_font();
}

void read_index_bin_handle_line(const char *line, const char *index_id, index_func func) {

    cJSON *document = cJSON_Parse(line);
    const char *path_md5_str = cJSON_GetObjectItem(document, "_id")->valuestring;

    cJSON_AddStringToObject(document, "index", index_id);

    // Load meta from sidecar files
    cJSON *meta_obj = NULL;
    if (IndexCtx.meta != NULL) {
        const char *meta_string = g_hash_table_lookup(IndexCtx.meta, path_md5_str);
        if (meta_string != NULL) {
            meta_obj = cJSON_Parse(meta_string);

            cJSON *child;
            for (child = meta_obj->child; child != NULL; child = child->next) {
                char meta_key[4096];
                strcpy(meta_key, child->string);
                cJSON_DeleteItemFromObject(document, meta_key);
                cJSON_AddItemReferenceToObject(document, meta_key, child);
            }
        }
    }

    // Load tags from tags DB
    if (IndexCtx.tags != NULL) {
        const char *tags_string = g_hash_table_lookup(IndexCtx.tags, path_md5_str);
        if (tags_string != NULL) {
            cJSON *tags_arr = cJSON_Parse(tags_string);
            cJSON_DeleteItemFromObject(document, "tag");
            cJSON_AddItemToObject(document, "tag", tags_arr);
        }
    }

    func(document, path_md5_str);
    cJSON_DeleteItemFromObject(document, "_id");
    cJSON_Delete(document);
    if (meta_obj) {
        cJSON_Delete(meta_obj);
    }
}

void read_lines(const char *path, const line_processor_t processor) {
    dyn_buffer_t buf = dyn_buffer_create();

    // Initialize zstd things
    FILE *file = fopen(path, "rb");

    size_t const buf_in_size = ZSTD_DStreamInSize();
    void *const buf_in = malloc(buf_in_size);

    size_t const buf_out_size = ZSTD_DStreamOutSize();
    void *const buf_out = malloc(buf_out_size);

    ZSTD_DCtx *const dctx = ZSTD_createDCtx();

    size_t read;
    size_t last_ret = 0;
    while ((read = fread(buf_in, 1, buf_in_size, file))) {
        ZSTD_inBuffer input = {buf_in, read, 0};

        while (input.pos < input.size) {
            ZSTD_outBuffer output = {buf_out, buf_out_size, 0};

            size_t const ret = ZSTD_decompressStream(dctx, &output, &input);

            for (int i = 0; i < output.pos; i++) {
                char c = ((char *) output.dst)[i];

                if (c == '\n') {
                    dyn_buffer_write_char(&buf, '\0');
                    processor.func(buf.buf, processor.data);
                    buf.cur = 0;
                } else {
                    dyn_buffer_write_char(&buf, c);
                }
            }

            last_ret = ret;
        }
    }

    if (last_ret != 0) {
        /* The last return value from ZSTD_decompressStream did not end on a
         * frame, but we reached the end of the file! We assume this is an
         * error, and the input was truncated.
         */
        LOG_FATALF("serialize.c", "EOF before end of stream: %zu", last_ret)
    }

    ZSTD_freeDCtx(dctx);
    free(buf_in);
    free(buf_out);

    dyn_buffer_destroy(&buf);
    fclose(file);

}

void read_index_ndjson(const char *line, void* _data) {
    void** data = _data;
    const char* index_id = data[0];
    index_func func = data[1];
    read_index_bin_handle_line(line, index_id, func);
}

void read_index(const char *path, const char index_id[MD5_STR_LENGTH], const char *type, index_func func) {
    if (strcmp(type, INDEX_TYPE_NDJSON) == 0) {
        read_lines(path, (line_processor_t) {
            .data = (void*[2]){(void*)index_id, func} ,
            .func = read_index_ndjson,
        });
    }
}

static __thread GHashTable *IncrementalReadTable = NULL;

void json_put_incremental(cJSON *document, UNUSED(const char id_str[MD5_STR_LENGTH])) {
    const char *path_md5_str = cJSON_GetObjectItem(document, "_id")->valuestring;
    const int mtime = cJSON_GetObjectItem(document, "mtime")->valueint;

    incremental_put_str(IncrementalReadTable, path_md5_str, mtime);
}

void incremental_read(GHashTable *table, const char *filepath, index_descriptor_t *desc) {
    IncrementalReadTable = table;
    read_index(filepath, desc->id, desc->type, json_put_incremental);
}

static __thread GHashTable *IncrementalCopyTable = NULL;
static __thread store_t *IncrementalCopySourceStore = NULL;
static __thread store_t *IncrementalCopyDestinationStore = NULL;

void incremental_copy_handle_doc(cJSON *document, UNUSED(const char id_str[MD5_STR_LENGTH])) {

    const char *path_md5_str = cJSON_GetObjectItem(document, "_id")->valuestring;
    unsigned char path_md5[MD5_DIGEST_LENGTH];
    hex2buf(path_md5_str, MD5_STR_LENGTH - 1, path_md5);

    if (cJSON_GetObjectItem(document, "parent") != NULL || incremental_get_str(IncrementalCopyTable, path_md5_str)) {
        // Copy index line
        cJSON_DeleteItemFromObject(document, "index");
        char *json_str = cJSON_PrintUnformatted(document);
        const size_t json_str_len = strlen(json_str);

        json_str = realloc(json_str, json_str_len + 1);
        *(json_str + json_str_len) = '\n';

        zstd_write_string(json_str, json_str_len + 1);
        free(json_str);

        // Copy tn store contents
        size_t buf_len;
        char *buf = store_read(IncrementalCopySourceStore, (char *) path_md5, sizeof(path_md5), &buf_len);
        if (buf_len != 0) {
            store_write(IncrementalCopyDestinationStore, (char *) path_md5, sizeof(path_md5), buf, buf_len);
            free(buf);
        }
    }
}

/**
 * Copy items from an index that are in the copy_table. Also copies from
 * the store.
 */
void incremental_copy(store_t *store, store_t *dst_store, const char *filepath,
                      const char *dst_filepath, GHashTable *copy_table) {

    if (WriterCtx.out_file == NULL) {
        initialize_writer_ctx(dst_filepath);
    }

    IncrementalCopyTable = copy_table;
    IncrementalCopySourceStore = store;
    IncrementalCopyDestinationStore = dst_store;

    read_index(filepath, "", INDEX_TYPE_NDJSON, incremental_copy_handle_doc);
}

void incremental_delete(const char *del_filepath, GHashTable *orig_table, GHashTable *new_table) {
    GHashTableIter iter;
    gpointer key, UNUSED(value);
    char path_md5[MD5_STR_LENGTH + 1];
    path_md5[MD5_STR_LENGTH] = '\0';
    path_md5[MD5_STR_LENGTH - 1] = '\n';
    initialize_writer_ctx(del_filepath);
    g_hash_table_iter_init(&iter, orig_table);
    while(g_hash_table_iter_next(&iter, &key, &value)) {
        if (NULL == g_hash_table_lookup(new_table, key)) {
            memcpy(path_md5, key, MD5_STR_LENGTH - 1);
            zstd_write_string(path_md5, MD5_STR_LENGTH);
        }
    }
    writer_cleanup();
}
