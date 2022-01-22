#ifndef SIST2_TPOOL_H
#define SIST2_TPOOL_H

#include "sist.h"

struct tpool;
typedef struct tpool tpool_t;

typedef void (*thread_func_t)(void *arg);

tpool_t *tpool_create(int num, void (*cleanup_func)(), int free_arg, int print_progress, size_t mem_limit);
void tpool_start(tpool_t *pool);
void tpool_destroy(tpool_t *pool);

int tpool_add_work(tpool_t *pool, thread_func_t func, void *arg);
void tpool_wait(tpool_t *pool);

void tpool_dump_debug_info(tpool_t *pool);

#endif


