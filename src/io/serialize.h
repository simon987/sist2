#ifndef SIST2_SERIALIZE_H
#define SIST2_SERIALIZE_H

#include "src/sist.h"
#include "store.h"

#include <sys/syscall.h>
#include <glib.h>

typedef void(*index_func)(cJSON *, const char[MD5_STR_LENGTH]);

void incremental_copy(store_t *store, store_t *dst_store, const char *filepath,
                      const char *dst_filepath, GHashTable *copy_table);

void write_document(document_t *doc);

void read_index(const char *path, const char[MD5_STR_LENGTH], const char *type, index_func);

void incremental_read(GHashTable *table, const char *filepath);

/**
 * Must be called after write_document
 */
void thread_cleanup();

void write_index_descriptor(char *path, index_descriptor_t *desc);

index_descriptor_t read_index_descriptor(char *path);

#endif