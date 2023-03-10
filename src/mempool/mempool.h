#ifndef SIST2_MEMPOOL_H
#define SIST2_MEMPOOL_H

#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <stdbool.h>

typedef unsigned char u_char;
typedef uintptr_t ncx_uint_t;

#ifndef NCX_ALIGNMENT
#define NCX_ALIGNMENT   sizeof(unsigned long)
#endif

#define ncx_align(d, a) (((d) + (a - 1)) & ~(a - 1))
#define ncx_align_ptr(p, a) (u_char *) (((uintptr_t) (p) + ((uintptr_t) a - 1)) & ~((uintptr_t) a - 1))

#define ncx_memzero(buf, n) (void) memset(buf, 0, n)
#define ncx_memset(buf, c, n) (void) memset(buf, c, n)

typedef struct ncx_slab_page_s ncx_slab_page_t;

struct ncx_slab_page_s {
    uintptr_t slab;
    ncx_slab_page_t *next;
    uintptr_t prev;
};

typedef struct {
    size_t min_size;
    size_t min_shift;

    ncx_slab_page_t *pages;
    ncx_slab_page_t free;

    u_char *start;
    u_char *end;

    //ncx_shmtx_t mutex;

    void *addr;
} ncx_slab_pool_t;

typedef struct {
    size_t pool_size, used_size, used_pct;
    size_t pages, free_page;
    size_t p_small, p_exact, p_big, p_page;
    size_t b_small, b_exact, b_big, b_page;
    size_t max_free_pages;
} ncx_slab_stat_t;

void ncx_slab_init(ncx_slab_pool_t *mempool);

void *ncx_slab_alloc(ncx_slab_pool_t *mempool, size_t size);

void ncx_slab_free(ncx_slab_pool_t *mempool, void *p);

void ncx_slab_stat(ncx_slab_pool_t *mempool, ncx_slab_stat_t *stat);

#endif //SIST2_MEMPOOL_H
