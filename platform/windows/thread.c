#include "../thread.h"
#include "core/log.h"
#include <core/int.h>
#include <core/util.h>
#include <core/vector.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <stdnoreturn.h>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif /* WIN32_LEAN_AND_MEAN */
#include <windows.h>
#include <winnt.h>
#include <process.h>
#include <synchapi.h>
#include <handleapi.h>
#include <minwinbase.h>
#include <processthreadsapi.h>
#define P_INTERNAL_GUARD__
#include "error.h"
#undef P_INTERNAL_GUARD__

#define MODULE_NAME "thread"

struct p_mt_mutex {
    CRITICAL_SECTION cs;
    _Atomic bool initialized;
    _Atomic bool is_static;
};

static bool should_not_destroy_master_mutex = false;
static struct p_mt_mutex master_mutex = {
    .initialized = false,
    .is_static = true,
};
static VECTOR(struct p_mt_mutex *) global_mutex_registry = NULL;
static volatile atomic_flag registered_atexit_cleanup = ATOMIC_FLAG_INIT;

struct p_mt_cond {
    CONDITION_VARIABLE cond;
};

static void add_mutex_to_registry(struct p_mt_mutex *m);
static void cleanup_global_mutexes(void);

i32 p_mt_thread_create(p_mt_thread_t *o,
    p_mt_thread_fn_t thread_fn, void *arg)
{
    u_check_params(o != NULL && thread_fn != NULL);

#define SECURITY_ATTRS NULL
#define STACK_SIZE 0
#define FLAGS 0
#define THREAD_ADDR_P NULL
    const union {
        p_mt_thread_fn_t p_mt;
        _beginthreadex_proc_type win32;
    } thread_fn_bridge = {
        .p_mt = thread_fn
    };
    *o = (HANDLE)_beginthreadex(SECURITY_ATTRS, STACK_SIZE,
        thread_fn_bridge.win32, arg,
        FLAGS, THREAD_ADDR_P
    );

    s_log_trace("Created new thread: %d", GetThreadId(*o));

    if (*o == NULL) {
        s_log_error("Failed to create thread: %s", strerror(errno));
        return 1;
    }

    return 0;
}

noreturn void p_mt_thread_exit(void)
{
    _endthreadex(0);
}

void p_mt_thread_wait(p_mt_thread_t *thread_p)
{
    u_check_params(thread_p != NULL && *thread_p != NULL);

    DWORD wait_ret = WaitForSingleObject(*thread_p, INFINITE);
    if (wait_ret != WAIT_OBJECT_0) {
        DWORD thread_id = GetThreadId(*thread_p);
        s_log_error("Failed to join thread %lu: %s",
            thread_id, get_last_error_msg());
        return;
    }

    CloseHandle(*thread_p);
    *thread_p = NULL;
}

void p_mt_thread_terminate(p_mt_thread_t *thread_p)
{
    u_check_params(thread_p != NULL);

    if (TerminateThread(*thread_p, 0) == 0) {
        DWORD thread_id = GetThreadId(*thread_p);
        s_log_error("Failed to terminate thread %lu: %s",
            thread_id, get_last_error_msg());
    }

    CloseHandle(*thread_p);
    *thread_p = NULL;
}

p_mt_mutex_t p_mt_mutex_create(void)
{
    struct p_mt_mutex *m = calloc(1, sizeof(struct p_mt_mutex));
    s_assert(m != NULL, "calloc() failed for struct mutex");

    InitializeCriticalSection(&m->cs);
    atomic_store(&m->initialized, true);
    atomic_store(&m->is_static, false);

    return m;
}

void p_mt_mutex_lock(p_mt_mutex_t *mutex_p)
{
    u_check_params(mutex_p != NULL);

    if (*mutex_p == P_MT_MUTEX_INITIALIZER) {
        *mutex_p = p_mt_mutex_create();
        atomic_store(&(*mutex_p)->is_static, true);
        add_mutex_to_registry(*mutex_p);
    } if (*mutex_p == NULL || !atomic_load(&(*mutex_p)->initialized)) {
        *mutex_p = p_mt_mutex_create();
    }

    EnterCriticalSection(&(*mutex_p)->cs);
}

void p_mt_mutex_unlock(p_mt_mutex_t *mutex_p)
{
    u_check_params(mutex_p != NULL);
    if (*mutex_p == NULL || !atomic_load(&(*mutex_p)->initialized))
        return;

    LeaveCriticalSection(&(*mutex_p)->cs);
}

void p_mt_mutex_destroy(p_mt_mutex_t *mutex_p)
{
    if (mutex_p == NULL || *mutex_p == NULL) return;

    struct p_mt_mutex *m = *mutex_p;
    if (!atomic_load(&m->initialized)) return;

    DeleteCriticalSection(&m->cs);

    atomic_store(&m->initialized, false);
    atomic_store(&m->is_static, false);
    u_nfree(mutex_p);
}

void p_mt_mutex_global_cleanup(void)
{
    if (atomic_load(&master_mutex.initialized))
        EnterCriticalSection(&master_mutex.cs);

    cleanup_global_mutexes();

    if (atomic_load(&master_mutex.initialized))
        LeaveCriticalSection(&master_mutex.cs);
}

p_mt_cond_t p_mt_cond_create(void)
{
    p_mt_cond_t cond = calloc(1, sizeof(struct p_mt_cond));
    s_assert(cond != NULL, "calloc() failed for new condition variable");

    InitializeConditionVariable(&cond->cond);

    return cond;
}

void p_mt_cond_wait(p_mt_cond_t cond, p_mt_mutex_t mutex)
{
    u_check_params(cond != NULL && mutex != NULL &&
        atomic_load(&mutex->initialized));
    SleepConditionVariableCS(&cond->cond, &mutex->cs, INFINITE);
}

void p_mt_cond_signal(p_mt_cond_t cond)
{
    u_check_params(cond != NULL);
    WakeConditionVariable(&cond->cond);
}

void p_mt_cond_destroy(p_mt_cond_t *cond_p)
{
    if (cond_p == NULL || *cond_p == NULL)
        return;

    u_nzfree(cond_p);
}

static void add_mutex_to_registry(struct p_mt_mutex *m)
{
    if (!atomic_load(&master_mutex.initialized)) {
        InitializeCriticalSection(&master_mutex.cs);
        atomic_store(&master_mutex.initialized, true);
    }

    EnterCriticalSection(&master_mutex.cs);

    if (!atomic_flag_test_and_set(&registered_atexit_cleanup)) {
        if (atexit(cleanup_global_mutexes)) {
            s_log_fatal("Failed to atexit() "
                "the global mutex cleanup function.");
        }
    }

    if (global_mutex_registry == NULL)
        global_mutex_registry = vector_new(struct p_mt_mutex *);

    vector_push_back(&global_mutex_registry, m);

    LeaveCriticalSection(&master_mutex.cs);
}

static void cleanup_global_mutexes(void)
{
    for (u32 i = 0; i < vector_size(global_mutex_registry); i++) {
        struct p_mt_mutex *curr_mutex = global_mutex_registry[i];

        DeleteCriticalSection(&curr_mutex->cs);
        atomic_store(&curr_mutex->initialized, false);
        atomic_store(&curr_mutex->is_static, false);

        free(curr_mutex);
        global_mutex_registry[i] = NULL;
    }

    s_log_debug("Cleaned up all (%u) global mutexes.",
        vector_size(global_mutex_registry)
    );
    vector_destroy(&global_mutex_registry);

    /* If we're called at exit, clean up the master mutex too */
    if (!should_not_destroy_master_mutex) {
        DeleteCriticalSection(&master_mutex.cs);
        atomic_store(&master_mutex.initialized, false);
    }
}
