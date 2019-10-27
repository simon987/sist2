#include "src/ctx.h"
#include "serialize.h"

static __thread int IndexFd = -1;

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
    cJSON_AddStringToObject(json, "rewrite_url", desc->rewrite_url);
    cJSON_AddNumberToObject(json, "timestamp", (double) desc->timestamp);

    int fd = open(path, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
    if (fd == -1) {
        perror(path);
    }
    char *str = cJSON_Print(json);
    write(fd, str, strlen(str));
    free(str);
    close(fd);

    cJSON_Delete(json);
}

index_descriptor_t read_index_descriptor(char *path) {

    struct stat info;
    stat(path, &info);
    int fd = open(path, O_RDONLY);
    char *buf = malloc(info.st_size + 1);
    read(fd, buf, info.st_size);
    *(buf + info.st_size) = '\0';
    close(fd);

    cJSON *json = cJSON_Parse(buf);

    index_descriptor_t descriptor;
    descriptor.timestamp = (long) cJSON_GetObjectItem(json, "timestamp")->valuedouble;
    strcpy(descriptor.root, cJSON_GetObjectItem(json, "root")->valuestring);
    strcpy(descriptor.name, cJSON_GetObjectItem(json, "name")->valuestring);
    strcpy(descriptor.rewrite_url, cJSON_GetObjectItem(json, "rewrite_url")->valuestring);
    descriptor.root_len = (short)strlen(descriptor.root);
    strcpy(descriptor.version, cJSON_GetObjectItem(json, "version")->valuestring);
    strcpy(descriptor.uuid, cJSON_GetObjectItem(json, "uuid")->valuestring);

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
        default:
            return NULL;
    }
}


void write_document(document_t *doc) {

    if (IndexFd == -1) {
        char dstfile[PATH_MAX];
        pid_t tid = syscall(SYS_gettid);
        snprintf(dstfile, PATH_MAX, "%s_index_%d", ScanCtx.index.path, tid);
        IndexFd = open(dstfile, O_CREAT | O_WRONLY | O_APPEND, S_IRUSR | S_IWUSR);

        if (IndexFd == -1) {
            perror("open");
        }
    }

    dyn_buffer_t buf = dyn_buffer_create();

    // Ignore root directory in the file path
    doc->ext = doc->ext - ScanCtx.index.desc.root_len;
    doc->base = doc->base - ScanCtx.index.desc.root_len;
    doc->filepath += ScanCtx.index.desc.root_len;

    dyn_buffer_write(&buf, doc, sizeof(line_t));
    dyn_buffer_write_str(&buf, doc->filepath);

    meta_line_t *meta = doc->meta_head;
    while (meta != NULL) {
        dyn_buffer_write_char(&buf, meta->key);

        if (IS_META_INT(meta->key)) {
            dyn_buffer_write_int(&buf, meta->intval);
        } else if (IS_META_LONG(meta->key)) {
            dyn_buffer_write_long(&buf, meta->longval);
        } else {
            dyn_buffer_write_str(&buf, meta->strval);
        }

        meta_line_t *tmp = meta;
        meta = meta->next;
        free(tmp);
    }
    dyn_buffer_write_char(&buf, '\n');

    write(IndexFd, buf.buf, buf.cur);
    ScanCtx.stat_index_size += buf.cur;
    dyn_buffer_destroy(&buf);
}

void serializer_cleanup() {
    close(IndexFd);
}

void read_index(const char *path, const char index_id[UUID_STR_LEN], index_func func) {

    line_t line;
    dyn_buffer_t buf = dyn_buffer_create();

    FILE *file = fopen(path, "rb");
    while (1) {
        buf.cur = 0;
        fread((void *) &line, 1, sizeof(line_t), file);
        if (feof(file)) {
            break;
        }

        cJSON *document = cJSON_CreateObject();
        cJSON_AddStringToObject(document, "index", index_id);

        char uuid_str[UUID_STR_LEN];
        uuid_unparse(line.uuid, uuid_str);

        cJSON_AddStringToObject(document, "mime", mime_get_mime_text(line.mime));
        cJSON_AddNumberToObject(document, "size", (double)line.size);
        cJSON_AddNumberToObject(document, "mtime", line.mtime);

        int c;
        while ((c = getc(file)) != 0) {
            dyn_buffer_write_char(&buf, (char) c);
        }
        dyn_buffer_write_char(&buf, '\0');

        cJSON_AddStringToObject(document, "extension", buf.buf + line.ext);
        if (*(buf.buf + line.ext - 1) == '.') {
            *(buf.buf + line.ext - 1) = '\0';
        } else {
            *(buf.buf + line.ext) = '\0';
        }
        cJSON_AddStringToObject(document, "name", buf.buf + line.base);
        *(buf.buf + line.base - 1) = '\0';
        cJSON_AddStringToObject(document, "path", buf.buf);

        enum metakey key = getc(file);
        while (key != '\n') {
            switch (key) {
                case MetaWidth:
                case MetaHeight:
                case MetaMediaDuration:
                case MetaMediaBitrate: {
                    int value;
                    fread(&value, sizeof(int), 1, file);
                    cJSON_AddNumberToObject(document, get_meta_key_text(key), value);
                    break;
                }
                case MetaMediaAudioCodec:
                case MetaMediaVideoCodec: {
                    int value;
                    fread(&value, sizeof(int), 1, file);
                    const AVCodecDescriptor *desc = avcodec_descriptor_get(value);
                    if (desc != NULL) {
                        cJSON_AddStringToObject(document, get_meta_key_text(key), desc->name);
                    }
                    break;
                }

                case MetaContent:
                case MetaArtist:
                case MetaAlbum:
                case MetaAlbumArtist:
                case MetaGenre:
                case MetaFontName:
                case MetaTitle: {
                    buf.cur = 0;
                    while ((c = getc(file)) != 0) {
                        if (!(SHOULD_IGNORE_CHAR(c)) || c == ' ') {
                            dyn_buffer_write_char(&buf, (char) c);
                        }
                    }
                    dyn_buffer_write_char(&buf, '\0');
                    cJSON_AddStringToObject(document, get_meta_key_text(key), buf.buf);
                    break;
                }
            }

            key = getc(file);
        }

        func(document, uuid_str);
        cJSON_free(document);
    }
    fclose(file);
}

void incremental_read(GHashTable *table, const char *filepath) {
    FILE *file = fopen(filepath, "rb");
    line_t line;

    while (1) {
        fread((void *) &line, 1, sizeof(line_t), file);
        if (feof(file)) {
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
        fread((void *) &line, 1, sizeof(line_t), file);
        if (feof(file)) {
            break;
        }

        if (incremental_get(copy_table, line.ino)) {
            fwrite(&line, sizeof(line), 1, dst_file);

            size_t buf_len;
            char *buf = store_read(store, (char *) line.uuid, 16, &buf_len);
            store_write(dst_store, (char *) line.uuid, 16, buf, buf_len);

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
                    fread(&val, sizeof(val), 1, file);
                    fwrite(&val, sizeof(val), 1, dst_file);
                } else if (IS_META_LONG(key)) {
                    long val;
                    fread(&val, sizeof(val), 1, file);
                    fwrite(&val, sizeof(val), 1, dst_file);
                } else {
                    while ((c = (char) getc(file))) {
                        fwrite(&c, sizeof(c), 1, dst_file);
                    }
                    fwrite("\0", sizeof(c), 1, dst_file);
                }
            }
        } else {
            skip_meta(file);
        }
    }
    fclose(file);
}
