#ifndef SIST2_TPOOL_H
#define SIST2_TPOOL_H

#include "sist.h"

struct tpool;
typedef struct tpool tpool_t;

typedef struct {
    size_t arg_size;
    void *arg;
} tpool_work_arg_t;

typedef struct {
    size_t arg_size;
    char arg[0];
} tpool_work_arg_shm_t;

typedef void (*thread_func_t)(tpool_work_arg_shm_t *arg);

tpool_t *tpool_create(int num, void (*cleanup_func)(), int print_progress, size_t mem_limit);

void tpool_start(tpool_t *pool);

void tpool_destroy(tpool_t *pool);

int tpool_add_work(tpool_t *pool, thread_func_t func, tpool_work_arg_t *arg);

void tpool_wait(tpool_t *pool);

void tpool_dump_debug_info(tpool_t *pool);

#endif


