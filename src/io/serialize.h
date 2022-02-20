#ifndef SIST2_SERIALIZE_H
#define SIST2_SERIALIZE_H

#include "src/sist.h"
#include "store.h"

#include <sys/syscall.h>
#include <glib.h>

typedef struct line_processor {
  void* data;
  void (*func)(const char*, void*);
} line_processor_t;

typedef void(*index_func)(cJSON *, const char[MD5_STR_LENGTH]);

void incremental_copy(store_t *store, store_t *dst_store, const char *filepath,
                      const char *dst_filepath, GHashTable *copy_table);

void incremental_delete(const char *del_filepath, const char* index_filepath, 
                        GHashTable *copy_table, GHashTable *new_table);

void write_document(document_t *doc);

void read_lines(const char *path, const line_processor_t processor);

void read_index(const char *path, const char[MD5_STR_LENGTH], const char *type, index_func);

void incremental_read(GHashTable *table, const char *filepath, index_descriptor_t *desc);

/**
 * Must be called after write_document
 */
void thread_cleanup();

void writer_cleanup();

void write_index_descriptor(char *path, index_descriptor_t *desc);

index_descriptor_t read_index_descriptor(char *path);

// caller ensures char file_path[PATH_MAX]
#define READ_INDICES(file_path, index_path, action_ok, action_main_fail, cond_original) \
    snprintf(file_path, PATH_MAX, "%s_index_main.ndjson.zst", index_path);              \
    if (0 == access(file_path, R_OK)) {                                                 \
        action_ok;                                                                      \
    } else {                                                                            \
        action_main_fail;                                                               \
    }                                                                                   \
    snprintf(file_path, PATH_MAX, "%s_index_original.ndjson.zst", index_path);          \
    if ((cond_original) && (0 == access(file_path, R_OK))) {                            \
        action_ok;                                                                      \
    }                                                                                   \


#endif
