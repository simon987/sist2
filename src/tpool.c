#include "tpool.h"
#include "ctx.h"

typedef void (*thread_func_t)(void *arg);

typedef struct tpool_work {
    void *arg;
    thread_func_t func;
    struct tpool_work *next;
} tpool_work_t;

typedef struct tpool {
    tpool_work_t *work_head;
    tpool_work_t *work_tail;

    pthread_mutex_t work_mutex;

    pthread_cond_t has_work_cond;
    pthread_cond_t working_cond;

    pthread_t *threads;

    int thread_cnt;
    int work_cnt;
    int done_cnt;

    int stop;

    void (*cleanup_func)();
} tpool_t;


/**
 * Create a work object
 */
static tpool_work_t *tpool_work_create(thread_func_t func, void *arg) {

    if (func == NULL) {
        return NULL;
    }

    tpool_work_t *work = malloc(sizeof(tpool_work_t));
    work->func = func;
    work->arg = arg;
    work->next = NULL;

    return work;
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
int tpool_add_work(tpool_t *pool, thread_func_t func, void *arg) {

    tpool_work_t *work = tpool_work_create(func, arg);
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

/**
 * Thread worker function
 */
static void *tpool_worker(void *arg) {
    tpool_t *pool = arg;

    while (1) {
        pthread_mutex_lock(&pool->work_mutex);
        if (pool->stop) {
            break;
        }

        if (pool->work_head == NULL) {
            pthread_cond_wait(&(pool->has_work_cond), &(pool->work_mutex));
        }

        tpool_work_t *work = tpool_work_get(pool);
        pthread_mutex_unlock(&(pool->work_mutex));

        if (work != NULL) {
            if (pool->stop) {
                break;
            }

            work->func(work->arg);
            free(work->arg);
            free(work);
        }

        pthread_mutex_lock(&(pool->work_mutex));
        if (work != NULL) {
            pool->done_cnt++;
        }

        progress_bar_print((double) pool->done_cnt / pool->work_cnt, ScanCtx.stat_tn_size, ScanCtx.stat_index_size);

        if (pool->work_head == NULL) {
            pthread_cond_signal(&(pool->working_cond));
        }
        pthread_mutex_unlock(&(pool->work_mutex));
    }

    LOG_INFO("tpool.c", "Executing cleaup function")
    pool->cleanup_func();

    pthread_cond_signal(&(pool->working_cond));
    pthread_mutex_unlock(&(pool->work_mutex));
    return NULL;
}

void tpool_wait(tpool_t *pool) {
    LOG_INFO("tpool.c", "Waiting for worker threads to finish")
    pthread_mutex_lock(&(pool->work_mutex));
    while (1) {
        if (pool->done_cnt < pool->work_cnt) {
            pthread_cond_wait(&(pool->working_cond), &(pool->work_mutex));
        } else {
            usleep(500000);
            if (pool->done_cnt == pool->work_cnt) {
                pool->stop = 1;
                usleep(1000000);
                break;
            }
        }
    }
    progress_bar_print(1.0, ScanCtx.stat_tn_size, ScanCtx.stat_index_size);
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
    while (work != NULL) {
        tpool_work_t *tmp = work->next;
        free(work);
        work = tmp;
    }

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

    free(pool->threads);
    free(pool);
}

/**
 * Create a thread pool
 * @param thread_cnt Worker threads count
 */
tpool_t *tpool_create(size_t thread_cnt, void cleanup_func()) {

    tpool_t *pool = malloc(sizeof(tpool_t));
    pool->thread_cnt = thread_cnt;
    pool->work_cnt = 0;
    pool->done_cnt = 0;
    pool->stop = 0;
    pool->cleanup_func = cleanup_func;
    pool->threads = calloc(sizeof(pthread_t), thread_cnt);

    pthread_mutex_init(&(pool->work_mutex), NULL);

    pthread_cond_init(&(pool->has_work_cond), NULL);
    pthread_cond_init(&(pool->working_cond), NULL);

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
