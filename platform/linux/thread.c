#define _GNU_SOURCE
#include "../thread.h"
#include <core/int.h>
#include <core/log.h>
#include <core/util.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>
#include <signal.h>

#define MODULE_NAME "thread"

struct p_mt_mutex {
    pthread_mutex_t mutex_handle;
    bool initialized;
};

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

    return m;
}

void p_mt_mutex_lock(p_mt_mutex_t mutex)
{
    u_check_params(mutex != NULL &&
        ((struct p_mt_mutex *)mutex)->initialized);

    pthread_mutex_lock(&((struct p_mt_mutex *)mutex)->mutex_handle);
}

void p_mt_mutex_unlock(p_mt_mutex_t mutex)
{
    u_check_params(mutex != NULL &&
        ((struct p_mt_mutex *)mutex)->initialized);

    pthread_mutex_unlock(&((struct p_mt_mutex *)mutex)->mutex_handle);
}

void p_mt_mutex_destroy(p_mt_mutex_t *mutex_p)
{
    if (mutex_p == NULL || *mutex_p == NULL) return;

    struct p_mt_mutex *m = *mutex_p;

    if (!m->initialized) return;
    pthread_mutex_destroy(&m->mutex_handle);
    
    /* Also sets `m->initialized` to false */
    memset(m, 0, sizeof(struct p_mt_mutex));
    u_nfree(mutex_p);
}
