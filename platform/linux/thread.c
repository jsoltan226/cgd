#define _GNU_SOURCE
#include "../thread.h"
#include <core/int.h>
#include <core/log.h>
#include <core/util.h>
#include <core/vector.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>
#include <signal.h>

#define MODULE_NAME "thread"

struct p_mt_mutex {
    pthread_mutex_t mutex_handle;
    bool initialized;
    bool is_static;
};

static pthread_mutex_t master_mutex = PTHREAD_MUTEX_INITIALIZER;
static VECTOR(struct p_mt_mutex *) global_mutex_registry = NULL;

static void add_mutex_to_registry(struct p_mt_mutex *m);

static bool registered_atexit_cleanup = false;
static void cleanup_global_mutexes(void);

i32 p_mt_thread_create(p_mt_thread_t *o,
    p_mt_thread_fn_t thread_fn, void *arg,
    u32 flags
)
{
    (void) flags;

    u_check_params(o != NULL && thread_fn != NULL);

    i32 ret = pthread_create((pthread_t *)o, NULL, thread_fn, arg);
    if (ret != 0)
        s_log_error("Failed to create thread: %s", strerror(ret));

    return ret;
}

void noreturn p_mt_thread_exit(void *ret)
{
    pthread_exit(ret);
}

void * p_mt_thread_wait(p_mt_thread_t *thread_p)
{
    u_check_params(thread_p != NULL);

    void *thread_ret = NULL;

    i32 join_ret = pthread_join(*(pthread_t *)thread_p, &thread_ret);
    if (join_ret != 0) {
        s_log_error("Failed to join thread: %s", strerror(join_ret));
        return NULL;
    }

    return thread_ret;
}

void p_mt_thread_terminate(p_mt_thread_t *thread_p)
{
    u_check_params(thread_p != NULL);
    i32 e = pthread_kill(*(pthread_t *)thread_p, SIGKILL);
    if (e != 0)
        s_log_error("Failed to kill thread: %s", strerror(e));
}

p_mt_mutex_t p_mt_mutex_create(void)
{
    struct p_mt_mutex *m = calloc(1, sizeof(struct p_mt_mutex));
    s_assert(m != NULL, "calloc() failed for struct mutex");

    /* Always returns 0 */
    (void) pthread_mutex_init(&m->mutex_handle, NULL);
    m->initialized = true;
    m->is_static = false;

    return m;
}

void p_mt_mutex_lock(p_mt_mutex_t *mutex_p)
{
    u_check_params(mutex_p != NULL);
    if (*mutex_p == P_MT_MUTEX_INITIALIZER) {
        *mutex_p = p_mt_mutex_create();
        (*mutex_p)->is_static = true;
        add_mutex_to_registry(*mutex_p);
    } else if (*mutex_p == NULL || !(*mutex_p)->initialized) {
        *mutex_p = p_mt_mutex_create();
    }

    pthread_mutex_lock(&(*mutex_p)->mutex_handle);
}

void p_mt_mutex_unlock(p_mt_mutex_t *mutex_p)
{
    u_check_params(mutex_p != NULL);
    if (*mutex_p == NULL || !(*mutex_p)->initialized)
        return;

    pthread_mutex_unlock(&(*mutex_p)->mutex_handle);
}

void p_mt_mutex_destroy(p_mt_mutex_t *mutex_p)
{
    if (mutex_p == NULL || *mutex_p == NULL) return;

    struct p_mt_mutex *m = *mutex_p;

    if (!m->initialized || m->is_static) return;

    /* If the mutex is locked, block until it gets unlocked */
    pthread_mutex_lock(&m->mutex_handle);

    /* Once we are sure that the mutex is not in use,
     * unlock it so that it can be destroyed */
    pthread_mutex_unlock(&m->mutex_handle);

    pthread_mutex_destroy(&m->mutex_handle);
    
    /* Also sets `m->initialized` to false */
    memset(m, 0, sizeof(struct p_mt_mutex));
    u_nfree(mutex_p);
}

void p_mt_mutex_global_cleanup()
{
    pthread_mutex_lock(&master_mutex);
    cleanup_global_mutexes();
    pthread_mutex_unlock(&master_mutex);
}

static void add_mutex_to_registry(struct p_mt_mutex *m)
{
    pthread_mutex_lock(&master_mutex);
    if (global_mutex_registry == NULL)
        global_mutex_registry = vector_new(struct p_mt_mutex *);

    if (!registered_atexit_cleanup) {
        s_log_debug("Registering global mutex cleanup function...");
        if (atexit(cleanup_global_mutexes)) {
            s_log_fatal(MODULE_NAME, __func__,
                "Failed to atexit() the global mutex cleanup function."
            );
        }
        registered_atexit_cleanup = true;
    }

    vector_push_back(global_mutex_registry, m);

    pthread_mutex_unlock(&master_mutex);
}

static void cleanup_global_mutexes(void)
{
    for (u32 i = 0; i < vector_size(global_mutex_registry); i++) {
        p_mt_mutex_destroy(&global_mutex_registry[i]);
    }
    s_log_debug("Cleaned up all (%u) global mutexes...",
        vector_size(global_mutex_registry)
    );

    vector_destroy(&global_mutex_registry);
}
