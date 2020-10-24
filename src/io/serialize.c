#include "src/ctx.h"
#include "serialize.h"
#include "src/parsing/parse.h"
#include "src/parsing/mime.h"

static __thread int index_fd = -1;

typedef struct {
    unsigned char uuid[16];
    unsigned long ino;
    unsigned long size;
    unsigned int mime;
    int mtime;
    short base;
    short ext;
} line_t;

void skip_meta(FILE *file) {
    enum metakey key = getc(file);
    while (key != '\n') {
        if (IS_META_INT(key)) {
            fseek(file, sizeof(int), SEEK_CUR);
        } else if (IS_META_LONG(key)) {
            fseek(file, sizeof(long), SEEK_CUR);
        } else {
            while ((getc(file))) {}
        }

        key = getc(file);
    }
}

void write_index_descriptor(char *path, index_descriptor_t *desc) {
    cJSON *json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "uuid", desc->uuid);
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
    int ret = write(fd, str, strlen(str));
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
    int ret = read(fd, buf, info.st_size);
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
    strcpy(descriptor.uuid, cJSON_GetObjectItem(json, "uuid")->valuestring);
    if (cJSON_GetObjectItem(json, "type") == NULL) {
        strcpy(descriptor.type, INDEX_TYPE_BIN);
    } else {
        strcpy(descriptor.type, cJSON_GetObjectItem(json, "type")->valuestring);
    }

    cJSON_Delete(json);
    free(buf);

    return descriptor;
}

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
        default:
            return NULL;
    }
}


void write_document(document_t *doc) {

    if (index_fd == -1) {
        char dstfile[PATH_MAX];
        pthread_t self = pthread_self();
        snprintf(dstfile, PATH_MAX, "%s_index_%lu", ScanCtx.index.path, self);
        index_fd = open(dstfile, O_CREAT | O_WRONLY | O_APPEND, S_IRUSR | S_IWUSR);

        if (index_fd == -1) {
            perror("open");
        }
    }

    dyn_buffer_t buf = dyn_buffer_create();

    // Ignore root directory in the file path
    doc->ext = (short) (doc->ext - ScanCtx.index.desc.root_len);
    doc->base = (short) (doc->base - ScanCtx.index.desc.root_len);
    doc->filepath += ScanCtx.index.desc.root_len;

    dyn_buffer_write(&buf, doc, sizeof(line_t));
    dyn_buffer_write_str(&buf, doc->filepath);

    meta_line_t *meta = doc->meta_head;
    while (meta != NULL) {
        dyn_buffer_write_char(&buf, meta->key);

        if (IS_META_INT(meta->key)) {
            dyn_buffer_write_int(&buf, meta->int_val);
        } else if (IS_META_LONG(meta->key)) {
            dyn_buffer_write_long(&buf, meta->long_val);
        } else {
            dyn_buffer_write_str(&buf, meta->str_val);
        }

        meta_line_t *tmp = meta;
        meta = meta->next;
        free(tmp);
    }
    dyn_buffer_write_char(&buf, '\n');

    int res = write(index_fd, buf.buf, buf.cur);
    if (res == -1) {
        LOG_FATALF("serialize.c", "Could not write document: %s", strerror(errno))
    }
    ScanCtx.stat_index_size += buf.cur;
    dyn_buffer_destroy(&buf);
}

void thread_cleanup() {
    close(index_fd);
    cleanup_parse();
    cleanup_font();
}


void read_index_bin(const char *path, const char *index_id, index_func func) {
    line_t line;
    dyn_buffer_t buf = dyn_buffer_create();

    FILE *file = fopen(path, "rb");
    while (1) {
        buf.cur = 0;
        size_t _ = fread((void *) &line, 1, sizeof(line_t), file);
        if (feof(file)) {
            break;
        }

        cJSON *document = cJSON_CreateObject();
        cJSON_AddStringToObject(document, "index", index_id);

        char uuid_str[UUID_STR_LEN];
        uuid_unparse(line.uuid, uuid_str);

        const char *mime_text = mime_get_mime_text(line.mime);
        if (mime_text == NULL) {
            cJSON_AddNullToObject(document, "mime");
        } else {
            cJSON_AddStringToObject(document, "mime", mime_get_mime_text(line.mime));
        }
        cJSON_AddNumberToObject(document, "size", (double) line.size);
        cJSON_AddNumberToObject(document, "mtime", line.mtime);

        int c = 0;
        while ((c = getc(file)) != 0) {
            dyn_buffer_write_char(&buf, (char) c);
        }
        dyn_buffer_write_char(&buf, '\0');

        char full_filename[PATH_MAX];
        strcpy(full_filename, buf.buf);

        cJSON_AddStringToObject(document, "extension", buf.buf + line.ext);
        if (*(buf.buf + line.ext - 1) == '.') {
            *(buf.buf + line.ext - 1) = '\0';
        } else {
            *(buf.buf + line.ext) = '\0';
        }

        char tmp[PATH_MAX * 3];

        str_escape(tmp, buf.buf + line.base);
        cJSON_AddStringToObject(document, "name", tmp);

        if (line.base > 0) {
            *(buf.buf + line.base - 1) = '\0';

            str_escape(tmp, buf.buf);
            cJSON_AddStringToObject(document, "path", tmp);
        } else {
            cJSON_AddStringToObject(document, "path", "");
        }

        enum metakey key = getc(file);
        size_t ret = 0;
        while (key != '\n') {
            switch (key) {
                case MetaPages:
                case MetaWidth:
                case MetaHeight: {
                    int value;
                    ret = fread(&value, sizeof(int), 1, file);
                    cJSON_AddNumberToObject(document, get_meta_key_text(key), value);
                    break;
                }
                case MetaMediaDuration:
                case MetaMediaBitrate: {
                    long value;
                    ret = fread(&value, sizeof(long), 1, file);
                    cJSON_AddNumberToObject(document, get_meta_key_text(key), (double) value);
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
                case MetaTitle: {
                    buf.cur = 0;
                    while ((c = getc(file)) != 0) {
                        if (SHOULD_KEEP_CHAR(c) || c == ' ') {
                            dyn_buffer_write_char(&buf, (char) c);
                        }
                    }
                    dyn_buffer_write_char(&buf, '\0');
                    cJSON_AddStringToObject(document, get_meta_key_text(key), buf.buf);
                    break;
                }
                default:
                LOG_FATALF("serialize.c", "Invalid meta key (corrupt index): %x", key)
            }

            key = getc(file);
        }

        cJSON *meta_obj = NULL;
        if (IndexCtx.meta != NULL) {
            const char *meta_string = g_hash_table_lookup(IndexCtx.meta, full_filename);
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

        if (IndexCtx.tags != NULL) {
            const char *tags_string = g_hash_table_lookup(IndexCtx.tags, full_filename);
            if (tags_string != NULL) {
                cJSON *tags_arr = cJSON_Parse(tags_string);
                cJSON_DeleteItemFromObject(document, "tag");
                cJSON_AddItemToObject(document, "tag", tags_arr);
            }
        }

        func(document, uuid_str);
        cJSON_Delete(document);
        if (meta_obj) {
            cJSON_Delete(meta_obj);
        }
    }
    dyn_buffer_destroy(&buf);
    fclose(file);
}

const char *json_type_copy_fields[] = {
        "mime", "name", "path", "extension", "index", "size", "mtime", "parent",

        // Meta
        "title", "content", "width", "height", "duration", "audioc", "videoc",
        "bitrate", "artist", "album", "album_artist", "genre", "title", "font_name",

        // Special
        "tag", "_url"
};

const char *json_type_array_fields[] = {
        "_keyword", "_text"
};

void read_index_json(const char *path, UNUSED(const char *index_id), index_func func) {

    FILE *file = fopen(path, "r");
    while (1) {
        char *line = NULL;
        size_t len;
        size_t read = getline(&line, &len, file);
        if (read < 0) {
            if (line) {
                free(line);
            }
            break;
        }

        cJSON *input = cJSON_Parse(line);
        if (input == NULL) {
            LOG_FATALF("serialize.c", "Could not parse JSON line: \n%s", line)
        }
        if (line) {
            free(line);
        }

        cJSON *document = cJSON_CreateObject();
        const char *uuid_str = cJSON_GetObjectItem(input, "_id")->valuestring;

        for (int i = 0; i < (sizeof(json_type_copy_fields) / sizeof(json_type_copy_fields[0])); i++) {
            cJSON *value = cJSON_GetObjectItem(input, json_type_copy_fields[i]);
            if (value != NULL) {
                cJSON_AddItemReferenceToObject(document, json_type_copy_fields[i], value);
            }
        }

        for (int i = 0; i < (sizeof(json_type_array_fields) / sizeof(json_type_array_fields[0])); i++) {
            cJSON *arr = cJSON_GetObjectItem(input, json_type_array_fields[i]);
            if (arr != NULL) {
                cJSON *obj;
                cJSON_ArrayForEach(obj, arr) {
                    char key[1024];
                    cJSON *k = cJSON_GetObjectItem(obj, "k");
                    cJSON *v = cJSON_GetObjectItem(obj, "v");
                    if (k == NULL || v == NULL || !cJSON_IsString(k) || !cJSON_IsString(v)) {
                        char *str = cJSON_Print(obj);
                        LOG_FATALF("serialize.c", "Invalid %s member: must contain .k and .v string fields: \n%s",
                                   json_type_array_fields[i], str)
                    }
                    snprintf(key, sizeof(key), "%s.%s", json_type_array_fields[i], k->valuestring);
                    cJSON_AddStringToObject(document, key, v->valuestring);
                }
            }
        }

        func(document, uuid_str);
        cJSON_Delete(document);
        cJSON_Delete(input);

    }
    fclose(file);
}

void read_index(const char *path, const char index_id[UUID_STR_LEN], const char *type, index_func func) {

    if (strcmp(type, INDEX_TYPE_BIN) == 0) {
        read_index_bin(path, index_id, func);
    } else if (strcmp(type, INDEX_TYPE_JSON) == 0) {
        read_index_json(path, index_id, func);
    }
}

void incremental_read(GHashTable *table, const char *filepath) {
    FILE *file = fopen(filepath, "rb");
    line_t line;

    while (1) {
        size_t ret = fread((void *) &line, 1, sizeof(line_t), file);
        if (ret != 1 || feof(file)) {
            break;
        }

        incremental_put(table, line.ino, line.mtime);

        while ((getc(file))) {}
        skip_meta(file);
    }
    fclose(file);
}

/**
 * Copy items from an index that are in the copy_table. Also copies from
 * the store.
 */
void incremental_copy(store_t *store, store_t *dst_store, const char *filepath,
                      const char *dst_filepath, GHashTable *copy_table) {
    FILE *file = fopen(filepath, "rb");
    FILE *dst_file = fopen(dst_filepath, "ab");
    line_t line;

    while (1) {
        size_t ret = fread((void *) &line, 1, sizeof(line_t), file);
        if (ret != 1 || feof(file)) {
            break;
        }

        if (incremental_get(copy_table, line.ino)) {
            fwrite(&line, sizeof(line), 1, dst_file);

            size_t buf_len;
            char *buf = store_read(store, (char *) line.uuid, 16, &buf_len);
            store_write(dst_store, (char *) line.uuid, 16, buf, buf_len);
            free(buf);

            char c;
            while ((c = (char) getc(file))) {
                fwrite(&c, sizeof(c), 1, dst_file);
            }
            fwrite("\0", sizeof(c), 1, dst_file);

            enum metakey key;
            while (1) {
                key = getc(file);
                if (key == '\n') {
                    break;
                }
                fwrite(&key, sizeof(char), 1, dst_file);

                if (IS_META_INT(key)) {
                    int val;
                    ret = fread(&val, sizeof(val), 1, file);
                    fwrite(&val, sizeof(val), 1, dst_file);
                } else if (IS_META_LONG(key)) {
                    long val;
                    ret = fread(&val, sizeof(val), 1, file);
                    fwrite(&val, sizeof(val), 1, dst_file);
                } else {
                    while ((c = (char) getc(file))) {
                        fwrite(&c, sizeof(c), 1, dst_file);
                    }
                    fwrite("\0", sizeof(c), 1, dst_file);
                }

                if (ret != 1) {
                    break;
                }
            }
        } else {
            skip_meta(file);
        }
    }
    fclose(file);
}
