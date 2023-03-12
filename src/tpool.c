#include "tpool.h"
#include "ctx.h"
#include "sist.h"
#include <pthread.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include "mempool/mempool.h"

#define BLANK_STR "                                                     "
// TODO: Use slab OOM to control queue size
#define MAX_QUEUE_SIZE 100000

typedef struct tpool_work {
    tpool_work_arg_shm_t *arg;
    thread_func_t func;
    struct tpool_work *next;
} tpool_work_t;

typedef struct tpool {
    tpool_work_t *work_head;
    tpool_work_t *work_tail;

    pthread_mutex_t work_mutex;
    pthread_mutex_t mem_mutex;

    // TODO: Initialize with SHARED attr
    pthread_cond_t has_work_cond;
    pthread_cond_t working_cond;

    pthread_t threads[256];

    int thread_cnt;
    int work_cnt;
    int done_cnt;
    int busy_cnt;

    int stop;
    int waiting;

    int print_progress;

    void (*cleanup_func)();

    void *shared_memory;
    size_t shared_memory_size;
    ncx_slab_pool_t *mempool;
} tpool_t;


/**
 * Create a work object
 */
static tpool_work_t *tpool_work_create(tpool_t *pool, thread_func_t func, tpool_work_arg_t *arg) {

    if (func == NULL) {
        return NULL;
    }

    // Copy heap arg to shm arg
    pthread_mutex_lock(&pool->mem_mutex);

    tpool_work_arg_shm_t *shm_arg = ncx_slab_alloc(pool->mempool, sizeof(tpool_work_arg_shm_t) + arg->arg_size);

    shm_arg->arg_size = arg->arg_size;
    memcpy(shm_arg->arg, arg->arg, arg->arg_size);

    free(arg->arg);

    tpool_work_t *work = ncx_slab_alloc(pool->mempool, sizeof(tpool_work_t));

    pthread_mutex_unlock(&pool->mem_mutex);

    work->func = func;
    work->arg = shm_arg;
    work->next = NULL;

    return work;
}

void tpool_dump_debug_info(tpool_t *pool) {
    LOG_DEBUGF("tpool.c", "pool->thread_cnt = %d", pool->thread_cnt)
    LOG_DEBUGF("tpool.c", "pool->work_cnt = %d", pool->work_cnt)
    LOG_DEBUGF("tpool.c", "pool->done_cnt = %d", pool->done_cnt)
    LOG_DEBUGF("tpool.c", "pool->busy_cnt = %d", pool->busy_cnt)
    LOG_DEBUGF("tpool.c", "pool->stop = %d", pool->stop)
}

/**
 * Pop work object from thread pool
 */
static tpool_work_t *tpool_work_get(tpool_t *pool) {

    tpool_work_t *work = pool->work_head;
    if (work == NULL) {
        return NULL;
    }

    if (work->next == NULL) {
        pool->work_head = NULL;
        pool->work_tail = NULL;
    } else {
        pool->work_head = work->next;
    }

    return work;
}

/**
 * Push work object to thread pool
 */
int tpool_add_work(tpool_t *pool, thread_func_t func, tpool_work_arg_t *arg) {

    while ((pool->work_cnt - pool->done_cnt) >= MAX_QUEUE_SIZE) {
        usleep(10000);
    }
    tpool_work_t *work = tpool_work_create(pool, func, arg);
    if (work == NULL) {
        return 0;
    }

    pthread_mutex_lock(&(pool->work_mutex));
    if (pool->work_head == NULL) {
        pool->work_head = work;
        pool->work_tail = pool->work_head;
    } else {
        pool->work_tail->next = work;
        pool->work_tail = work;
    }

    pool->work_cnt++;

    pthread_cond_broadcast(&(pool->has_work_cond));
    pthread_mutex_unlock(&(pool->work_mutex));

    return 1;
}

static void worker_thread_loop(tpool_t *pool) {
    while (TRUE) {
        pthread_mutex_lock(&pool->work_mutex);
        if (pool->stop) {
            break;
        }

        if (pool->work_head == NULL) {
            pthread_cond_wait(&(pool->has_work_cond), &(pool->work_mutex));
        }

        tpool_work_t *work = tpool_work_get(pool);

        if (work != NULL) {
            pool->busy_cnt += 1;
        }

        pthread_mutex_unlock(&(pool->work_mutex));

        if (work != NULL) {
            if (pool->stop) {
                break;
            }

            work->func(work->arg);

            pthread_mutex_lock(&pool->mem_mutex);
            ncx_slab_free(pool->mempool, work->arg);
            ncx_slab_free(pool->mempool, work);
            pthread_mutex_unlock(&pool->mem_mutex);
        }

        pthread_mutex_lock(&(pool->work_mutex));
        if (work != NULL) {
            pool->busy_cnt -= 1;
            pool->done_cnt++;
        }

        if (pool->print_progress) {
            if (LogCtx.json_logs) {
                progress_bar_print_json(pool->done_cnt, pool->work_cnt, ScanCtx.stat_tn_size,
                                        ScanCtx.stat_index_size, pool->waiting);
            } else {
                progress_bar_print((double) pool->done_cnt / pool->work_cnt, ScanCtx.stat_tn_size,
                                   ScanCtx.stat_index_size);
            }
        }

        if (pool->work_head == NULL) {
            pthread_cond_signal(&(pool->working_cond));
        }
        pthread_mutex_unlock(&(pool->work_mutex));
    }
}

/**
 * Thread worker function
 */
static void *tpool_worker(void *arg) {
    tpool_t *pool = arg;

    int pid = fork();

    if (pid == 0) {

        worker_thread_loop(pool);

        if (pool->cleanup_func != NULL) {
            LOG_INFO("tpool.c", "Executing cleanup function")
            pool->cleanup_func();
            LOG_DEBUG("tpool.c", "Done executing cleanup function")
        }

        pthread_cond_signal(&(pool->working_cond));
        pthread_mutex_unlock(&(pool->work_mutex));
        exit(0);

    } else {
        int status;
        // TODO: On crash, print debug info and resume thread
        waitpid(pid, &status, 0);

        LOG_DEBUGF("tpool.c", "Child process terminated with status code %d", WEXITSTATUS(status))

        pthread_mutex_lock(&(pool->work_mutex));
        pool->busy_cnt -= 1;
        pool->done_cnt++;
        pthread_mutex_unlock(&(pool->work_mutex));

        if (WIFSIGNALED(status)) {
//            parse_job_t *job = g_hash_table_lookup(ScanCtx.dbg_current_files, GINT_TO_POINTER(pthread_self()));
            const char *job_filepath = "TODO";

            LOG_FATALF_NO_EXIT(
                    "tpool.c",
                    "Child process was terminated by signal (%s).\n"
                            BLANK_STR "The process was working on %s",
                    strsignal(WTERMSIG(status)),
                    job_filepath
            )
        }
    }

    return NULL;
}

void tpool_wait(tpool_t *pool) {
    LOG_DEBUG("tpool.c", "Waiting for worker threads to finish")
    pthread_mutex_lock(&(pool->work_mutex));

    pool->waiting = TRUE;

    while (TRUE) {
        if (pool->done_cnt < pool->work_cnt) {
            pthread_cond_wait(&(pool->working_cond), &(pool->work_mutex));
        } else {
            LOG_INFOF("tpool.c", "Received head=NULL signal, busy_cnt=%d", pool->busy_cnt);

            if (pool->done_cnt == pool->work_cnt && pool->busy_cnt == 0) {
                pool->stop = TRUE;
                break;
            }
        }
    }
    if (pool->print_progress && !LogCtx.json_logs) {
        progress_bar_print(1.0, ScanCtx.stat_tn_size, ScanCtx.stat_index_size);
    }
    pthread_mutex_unlock(&(pool->work_mutex));

    LOG_INFO("tpool.c", "Worker threads finished")
}

void tpool_destroy(tpool_t *pool) {
    if (pool == NULL) {
        return;
    }

    LOG_INFO("tpool.c", "Destroying thread pool")

    pthread_mutex_lock(&(pool->work_mutex));
    tpool_work_t *work = pool->work_head;
    int count = 0;
    while (work != NULL) {
        tpool_work_t *tmp = work->next;
        free(work);
        work = tmp;
        count += 1;
    }

    LOG_DEBUGF("tpool.c", "Destroyed %d jobs", count);

    pthread_cond_broadcast(&(pool->has_work_cond));
    pthread_mutex_unlock(&(pool->work_mutex));

    for (size_t i = 0; i < pool->thread_cnt; i++) {
        pthread_t thread = pool->threads[i];
        if (thread != 0) {
            void *_;
            pthread_join(thread, &_);
        }
    }

    LOG_INFO("tpool.c", "Final cleanup")

    pthread_mutex_destroy(&(pool->work_mutex));
    pthread_cond_destroy(&(pool->has_work_cond));
    pthread_cond_destroy(&(pool->working_cond));

    munmap(pool->shared_memory, pool->shared_memory_size);
}

/**
 * Create a thread pool
 * @param thread_cnt Worker threads count
 */
tpool_t *tpool_create(int thread_cnt, void cleanup_func(), int print_progress) {

    size_t shm_size = 1024 * 1024 * 2000;

    void *shared_memory = mmap(NULL, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    tpool_t *pool = (tpool_t *) shared_memory;
    pool->shared_memory = shared_memory;
    pool->shared_memory_size = shm_size;
    pool->mempool = (ncx_slab_pool_t *) (pool->shared_memory + sizeof(tpool_t));
    pool->mempool->addr = pool->mempool;
    pool->mempool->min_shift = 4;
    pool->mempool->end = pool->shared_memory + shm_size;

    ncx_slab_init(pool->mempool);

    pool->thread_cnt = thread_cnt;
    pool->work_cnt = 0;
    pool->done_cnt = 0;
    pool->busy_cnt = 0;
    pool->stop = FALSE;
    pool->waiting = FALSE;
    pool->cleanup_func = cleanup_func;
    memset(pool->threads, 0, sizeof(pool->threads));
    pool->print_progress = print_progress;

    pthread_mutexattr_t mutexattr;
    pthread_mutexattr_init(&mutexattr);
    pthread_mutexattr_setpshared(&mutexattr, TRUE);

    pthread_mutex_init(&(pool->work_mutex), &mutexattr);
    pthread_mutex_init(&(pool->mem_mutex), &mutexattr);

    pthread_condattr_t condattr;
    pthread_condattr_init(&condattr);
    pthread_condattr_setpshared(&condattr, TRUE);

    pthread_cond_init(&(pool->has_work_cond), &condattr);
    pthread_cond_init(&(pool->working_cond), &condattr);

    pool->work_head = NULL;
    pool->work_tail = NULL;

    return pool;
}

void tpool_start(tpool_t *pool) {

    LOG_INFOF("tpool.c", "Starting thread pool with %d threads", pool->thread_cnt)

    for (size_t i = 0; i < pool->thread_cnt; i++) {
        pthread_create(&pool->threads[i], NULL, tpool_worker, pool);
    }
}
