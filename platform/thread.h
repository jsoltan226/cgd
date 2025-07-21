#ifndef P_THREAD_H_
#define P_THREAD_H_

#include <core/int.h>
#include <stdnoreturn.h>

/* Normally I would call all of these functions
 * `p_thread_create`, `p_thread_kill` etc,
 * but I wanted to avoid confusion with the UNIX `pthread` API,
 * so everything is named `p_mt_*` ("platform multi-threading") instead.
 */

/** THREADS **/
typedef void * p_mt_thread_t;

typedef void (*p_mt_thread_fn_t)(void *arg);

/* Spawn a thread with the starting routine `thread_fn`,
 * invoked with the parameters `arg`,
 * writing the handle to `o`.
 * Returns 0 on success and non-zero on failure. */
i32 p_mt_thread_create(p_mt_thread_t *o,
    p_mt_thread_fn_t thread_fn, void *arg);

/* Exit from a thread */
noreturn void p_mt_thread_exit(void);

/* Waits for thread pointed to by `thread_p` to terminate. */
void p_mt_thread_wait(p_mt_thread_t *thread_p);

/* Forcibly, immidiately terminates `thread`. */
void p_mt_thread_terminate(p_mt_thread_t *thread_p);

/** MUTEXES **/
struct p_mt_mutex;
typedef struct p_mt_mutex * p_mt_mutex_t;

/* Should be used for static (global) mutexes ONLY.
 * If a static mutex is initialized to this value,
 * it will be autmatically cleaned up when the program exits. */
#define P_MT_MUTEX_INITIALIZER ((void *)-1)

/* A value that can be used to invalidate a mutex handle */
#define P_MT_MUTEX_NULL ((p_mt_mutex_t)(void *)0)

/* Returns a newly allocated mutex. Always succeeds. */
p_mt_mutex_t p_mt_mutex_create(void);

/* Locks the mutex pointed to by `mutex_p`.
 *
 * If it's NULL, a new mutex is allocated and `*mutex_p` is set to point to it.
 *
 * If the value of `*mutex_p` is equal to `P_MT_MUTEX_INITIALIZER`,
 * then in addition to the above, the new mutex is also registered
 * for automatic cleanup at program exit. */
void p_mt_mutex_lock(p_mt_mutex_t *mutex_p);

/* Unlocks the mutex that `mutex_p` points to.
 * If `mutex_p` points to NULL, nothing happens. */
void p_mt_mutex_unlock(p_mt_mutex_t *mutex_p);

/* Deallocates the mutex that `mutex_p` points to. */
void p_mt_mutex_destroy(p_mt_mutex_t *mutex_p);

/* Deallocates all global mutexes initialized with `P_MT_MUTEX_INITIALIZER`.
 * Shouldn't be used outside of testing as it might leave dangling pointers,
 * plus it isn't really useful anyway. */
void p_mt_mutex_global_cleanup(void);

/** CONDITION VARIABLES **/
struct p_mt_cond;
typedef struct p_mt_cond * p_mt_cond_t;

/* A value that can be used to invalidate a condition variable handle */
#define P_MT_COND_NULL ((p_mt_cond_t)(void *)0)

/* Returns a new condition variable. Always succeeds. */
p_mt_cond_t p_mt_cond_create(void);

/* Causes the current thread to block until the condition variable
 * `cond` is signaled (using `p_mt_cond_signal`) from another thread.
 * This function must be called when `mutex` is locked.
 * It will also automatically unlock `mutex` prior to returning. */
void p_mt_cond_wait(p_mt_cond_t cond, p_mt_mutex_t mutex);

/* Wakes up all threads blocked on waiting (`p_mt_cond_wait`)
 * for the condition variable `cond` */
void p_mt_cond_signal(p_mt_cond_t cond);

/* Destroys the condition variable that `cond_p` points to. */
void p_mt_cond_destroy(p_mt_cond_t *cond_p);

#endif /* P_THREAD_H_ */
