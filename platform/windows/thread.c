#include "../thread.h"
#include "core/log.h"
#include <core/int.h>
#include <core/util.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdnoreturn.h>
#include <windows.h>
#include <winnt.h>
#include <process.h>
#include <synchapi.h>
#include <handleapi.h>
#include <minwinbase.h>
#include <errhandlingapi.h>
#include <processthreadsapi.h>

#define MODULE_NAME "thread"

struct p_mt_mutex {
    CRITICAL_SECTION cs;
    bool initialized;
};

i32 p_mt_thread_create(p_mt_thread_t *o,
    p_mt_thread_fn_t thread_fn, void *arg,
    u32 flags
)
{
    u_check_params(o != NULL && thread_fn != NULL);

#define SECURITY_ATTRS NULL
#define STACK_SIZE 0
#define FLAGS 0
#define THREAD_ADDR_P NULL
    *o = (HANDLE)_beginthreadex(SECURITY_ATTRS, STACK_SIZE,
        (_beginthreadex_proc_type)thread_fn, arg,
        FLAGS, THREAD_ADDR_P
    );

    if (*o == NULL) {
        s_log_error("Failed to create thread: %s", strerror(errno));
        return 1;
    }

    return 0;
}

void noreturn p_mt_thread_exit(void *ret)
{
    _endthreadex((u64)ret);
}

void * p_mt_thread_wait(p_mt_thread_t *thread_p)
{
    DWORD wait_ret = WaitForSingleObject(*thread_p, INFINITE);
    if (wait_ret != WAIT_OBJECT_0) {
        s_log_error("Failed to join thread: %s", GetLastError());
        return NULL;
    }

    DWORD exit_code = 0;
    (void) GetExitCodeThread(*thread_p, &exit_code);
    CloseHandle(*thread_p);

    *thread_p = NULL;
    return (void *)(u64)exit_code;
}

void p_mt_thread_terminate(p_mt_thread_t *thread_p)
{
    u_check_params(thread_p != NULL);

    if (TerminateThread(*thread_p, 0) == 0)
        s_log_error("Failed to terminate thread: %s", GetLastError());

    CloseHandle(*thread_p);
    *thread_p = NULL;
}

p_mt_mutex_t p_mt_mutex_create(void)
{
    struct p_mt_mutex *m = calloc(1, sizeof(struct p_mt_mutex));
    s_assert(m != NULL, "calloc() failed for struct mutex");

    InitializeCriticalSection(&m->cs);
    m->initialized = true;

    return m;
}

void p_mt_mutex_lock(p_mt_mutex_t mutex)
{
    u_check_params(mutex != NULL &&
        ((struct p_mt_mutex *)mutex)->initialized);

    EnterCriticalSection(&((struct p_mt_mutex *)mutex)->cs);
}

void p_mt_mutex_unlock(p_mt_mutex_t mutex)
{
    u_check_params(mutex != NULL &&
        ((struct p_mt_mutex *)mutex)->initialized);

    LeaveCriticalSection(&((struct p_mt_mutex *)mutex)->cs);
}

void p_mt_mutex_destroy(p_mt_mutex_t *mutex_p)
{
    if (mutex_p == NULL || *mutex_p == NULL) return;

    struct p_mt_mutex *m = *mutex_p;
    if (!m->initialized) return;

    DeleteCriticalSection(&m->cs);

    /* Also sets `m->initialized` to false */
    memset(*mutex_p, 0, sizeof(struct p_mt_mutex));
    u_nfree(mutex_p);
}
