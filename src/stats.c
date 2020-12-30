#include "sist.h"
#include "io/serialize.h"
#include "ctx.h"

static GHashTable *FlatTree;
static GHashTable *BufferTable;

static GHashTable *AggMime;
static GHashTable *AggSize;
static GHashTable *AggDate;

#define SIZE_BUCKET (long)(5 * 1024 * 1024)
#define DATE_BUCKET (long)(2629800)

static long TotalSize = 0;
static long DocumentCount = 0;

typedef struct {
    long size;
    long count;
} agg_t;

void fill_tables(cJSON *document, UNUSED(const char index_id[MD5_STR_LENGTH])) {

    if (cJSON_GetObjectItem(document, "parent") != NULL) {
        return;
    }

    const char *json_path = cJSON_GetObjectItem(document, "path")->valuestring;
    char *path = malloc(strlen(json_path) + 1);
    strcpy(path, json_path);

    const char *json_mime = cJSON_GetObjectItem(document, "mime")->valuestring;
    char *mime;
    if (json_mime == NULL) {
        mime = NULL;
    } else {
        mime = malloc(strlen(json_mime) + 1);
        strcpy(mime, json_mime);
    }

    long size = (long) cJSON_GetObjectItem(document, "size")->valuedouble;
    int mtime = cJSON_GetObjectItem(document, "mtime")->valueint;

    // treemap
    void *existing_path = g_hash_table_lookup(FlatTree, path);
    if (existing_path == NULL) {
        g_hash_table_insert(FlatTree, path, (gpointer) size);
    } else {
        g_hash_table_replace(FlatTree, path, (gpointer) ((long) existing_path + size));
    }

    // mime agg
    if (mime != NULL) {
        agg_t *orig_agg = g_hash_table_lookup(AggMime, mime);
        if (orig_agg == NULL) {
            agg_t *agg = malloc(sizeof(agg_t));
            agg->size = size;
            agg->count = 1;
            g_hash_table_insert(AggMime, mime, agg);
        } else {
            orig_agg->size += size;
            orig_agg->count += 1;
            free(mime);
        }
    }

    // size agg
    long size_bucket = size - (size % SIZE_BUCKET);
    agg_t *orig_agg = g_hash_table_lookup(AggSize, (gpointer) size_bucket);
    if (orig_agg == NULL) {
        agg_t *agg = malloc(sizeof(agg_t));
        agg->size = size;
        agg->count = 1;
        g_hash_table_insert(AggSize, (gpointer) size_bucket, agg);
    } else {
        orig_agg->count += 1;
        orig_agg->size += size;
    }

    // date agg
    long date_bucket = mtime - (mtime % DATE_BUCKET);
    orig_agg = g_hash_table_lookup(AggDate, (gpointer) date_bucket);
    if (orig_agg == NULL) {
        agg_t *agg = malloc(sizeof(agg_t));
        agg->size = size;
        agg->count = 1;
        g_hash_table_insert(AggDate, (gpointer) date_bucket, agg);
    } else {
        orig_agg->count += 1;
        orig_agg->size += size;
    }

    TotalSize += size;
    DocumentCount += 1;
}

void read_index_into_tables(index_t *index) {
    DIR *dir = opendir(index->path);
    struct dirent *de;
    while ((de = readdir(dir)) != NULL) {
        if (strncmp(de->d_name, "_index_", sizeof("_index_") - 1) == 0) {
            char file_path[PATH_MAX];
            snprintf(file_path, PATH_MAX, "%s%s", index->path, de->d_name);
            read_index(file_path, index->desc.id, index->desc.type, fill_tables);
        }
    }
    closedir(dir);
}

static size_t rfind(const char *str, int c) {
    for (int i = (int)strlen(str); i >= 0; i--) {
        if (str[i] == c) {
            return i;
        }
    }
    return -1;
}

int merge_up(double thresh) {
    long min_size = (long) (thresh * (double) TotalSize);

    int count = 0;
    GHashTableIter iter;
    g_hash_table_iter_init(&iter, FlatTree);

    void *key;
    void *value;

    while (g_hash_table_iter_next(&iter, &key, &value)) {
        long size = (long) value;

        if (size < min_size) {
            int stop = rfind(key, '/');
            if (stop == -1) {
                stop = 0;
            }
            char *parent = malloc(stop + 1);
            strncpy(parent, key, stop);
            *(parent + stop) = '\0';

            void *existing_parent = g_hash_table_lookup(FlatTree, parent);
            if (existing_parent == NULL) {
                void *existing_parent2_key;
                void *existing_parent2_val;
                int found = g_hash_table_lookup_extended(BufferTable, parent, &existing_parent2_key,
                                                         &existing_parent2_val);
                if (!found) {
                    g_hash_table_insert(BufferTable, parent, value);
                } else {
                    g_hash_table_replace(BufferTable, parent, (gpointer) ((long) existing_parent2_val + size));
                    free(existing_parent2_key);
                }
            } else {
                g_hash_table_replace(FlatTree, parent, (gpointer) ((long) existing_parent + size));
            }

            g_hash_table_iter_remove(&iter);

            count += 1;
        }
    }

    g_hash_table_iter_init(&iter, BufferTable);
    while (g_hash_table_iter_next(&iter, &key, &value)) {
        g_hash_table_insert(FlatTree, key, value);
        g_hash_table_iter_remove(&iter);
    }

    int size = g_hash_table_size(FlatTree);

    LOG_DEBUGF("stats.c", "Merge up iteration (%d merged, %d in tree)", count, size)
    return count;
}

/**
 * Assumes out is at at least PATH_MAX *4
 */
void csv_escape(char *dst, const char *str) {

    const char *ptr = str;
    char *out = dst;

    if (rfind(str, ',') == -1 && rfind(str, '"') == -1) {
        strcpy(dst, str);
        return;
    }

    *out++ = '"';
    char c;
    while ((c = *ptr++) != 0) {
        if (c == '"') {
            *out++ = '"';
            *out++ = '"';
        } else {
            *out++ = c;
        }
    }
    *out++ = '"';
    *out = '\0';
}

int open_or_exit(const char *path) {
    int fd = open(path, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
    if (fd < 0) {
        LOG_FATALF("stats.c", "Error while creating file: %s [%d]\n", strerror(errno), errno)
    }
    return fd;
}

#define TREEMAP_CSV_HEADER "path,size"
#define MIME_AGG_CSV_HEADER "mime,size,count"
#define SIZE_AGG_CSV_HEADER "bucket,size,count"
#define DATE_AGG_CSV_HEADER "bucket,size,count"

void write_treemap_csv(double thresh, const char *out_path) {

    void *key;
    void *value;

    long min_size = (long) (thresh * (double) TotalSize);

    int fd = open_or_exit(out_path);
    int ret = write(fd, TREEMAP_CSV_HEADER, sizeof(TREEMAP_CSV_HEADER) - 1);
    if (ret == -1) {
        LOG_FATALF("stats.c", "Write error: %s", strerror(errno))
    }

    GHashTableIter iter;
    g_hash_table_iter_init(&iter, FlatTree);
    while (g_hash_table_iter_next(&iter, &key, &value)) {
        long size = (long) value;

        if (size >= min_size) {
            char path_buf[PATH_MAX * 4];
            char buf[PATH_MAX * 4 + 16];

            csv_escape(path_buf, key);
            size_t written = sprintf(buf, "\n%s,%ld", path_buf, (long) value);
            ret = write(fd, buf, written);
            if (ret == -1) {
                LOG_FATALF("stats.c", "Write error: %s", strerror(errno))
            }
        }
    }
    close(fd);
}

void write_agg_csv_str(const char *out_path, const char *header, GHashTable *table) {
    void *key;
    void *value;
    char buf[4096];

    int fd = open_or_exit(out_path);
    int ret = write(fd, header, strlen(header));
    if (ret == -1) {
        LOG_FATALF("stats.c", "Write error: %s", strerror(errno))
    }

    GHashTableIter iter;
    g_hash_table_iter_init(&iter, table);
    while (g_hash_table_iter_next(&iter, &key, &value)) {
        agg_t *agg = value;

        size_t written = sprintf(buf, "\n%s,%ld,%ld", (const char*)key, agg->size, agg->count);
        ret = write(fd, buf, written);
        if (ret == -1) {
            LOG_FATALF("stats.c", "Write error: %s", strerror(errno))
        }
    }

    close(fd);
}

void write_agg_csv_long(const char *out_path, const char *header, GHashTable *table) {
    void *key;
    void *value;
    char buf[4096];

    int fd = open_or_exit(out_path);
    int ret = write(fd, header, strlen(header));
    if (ret == -1) {
        LOG_FATALF("stats.c", "Write error: %s", strerror(errno))
    }

    GHashTableIter iter;
    g_hash_table_iter_init(&iter, table);
    while (g_hash_table_iter_next(&iter, &key, &value)) {
        agg_t *agg = value;
        size_t written = sprintf(buf, "\n%ld,%ld,%ld", (long)key, agg->size, agg->count);
        ret = write(fd, buf, written);
        if (ret == -1) {
            LOG_FATALF("stats.c", "Write error: %s", strerror(errno))
        }
    }

    close(fd);
}

int generate_stats(index_t *index, const double threshold, const char *out_prefix) {

    FlatTree = g_hash_table_new_full(g_str_hash, g_str_equal, free, NULL);
    BufferTable = g_hash_table_new(g_str_hash, g_str_equal);

    AggMime = g_hash_table_new_full(g_str_hash, g_str_equal, free, free);
    AggSize = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, free);
    AggDate = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, free);

    LOG_INFO("stats.c", "Generating stats...")

    read_index_into_tables(index);

    LOG_DEBUG("stats.c", "Read index into tables")
    LOG_DEBUGF("stats.c", "Total size is %ld", TotalSize)
    LOG_DEBUGF("stats.c", "Document count is %ld", DocumentCount)
    LOG_DEBUGF("stats.c", "Merging small directories upwards with a threshold of %f%%", threshold * 100)

    while (merge_up(threshold) > 100) {}

    char tmp[PATH_MAX];

    strncpy(tmp, out_prefix, sizeof(tmp));
    strcat(tmp, "treemap.csv");
    write_treemap_csv(threshold, tmp);

    strncpy(tmp, out_prefix, sizeof(tmp));
    strcat(tmp, "mime_agg.csv");
    write_agg_csv_str(tmp, MIME_AGG_CSV_HEADER, AggMime);

    strncpy(tmp, out_prefix, sizeof(tmp));
    strcat(tmp, "size_agg.csv");
    write_agg_csv_long(tmp, SIZE_AGG_CSV_HEADER, AggSize);

    strncpy(tmp, out_prefix, sizeof(tmp));
    strcat(tmp, "date_agg.csv");
    write_agg_csv_long(tmp, DATE_AGG_CSV_HEADER, AggDate);

    g_hash_table_remove_all(FlatTree);
    g_hash_table_destroy(FlatTree);
    g_hash_table_destroy(BufferTable);

    g_hash_table_remove_all(AggMime);
    g_hash_table_destroy(AggMime);
    g_hash_table_remove_all(AggSize);
    g_hash_table_destroy(AggSize);
    g_hash_table_remove_all(AggDate);
    g_hash_table_destroy(AggDate);

    return 0;
}

