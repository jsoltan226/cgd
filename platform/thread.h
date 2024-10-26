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

typedef void * (*p_mt_thread_fn_t)(void *arg);

/* Spawn a thread with the starting routine `thread_fn`,
 * invoked with the parameters `arg`,
 * writing the handle to `o`.
 * `flags` is currently unused and can be 0.
 * Returns 0 on success and non-zero on failure. */
i32 p_mt_thread_create(p_mt_thread_t *o,
    p_mt_thread_fn_t thread_fn, void *arg,
    u32 flags
);

/* Exit from a thread */
void noreturn p_mt_thread_exit(void *ret);

/* Waits for thread `thread` to terminate.
 * Returns whatever the thread returned on exit. */
void * p_mt_thread_wait(p_mt_thread_t *thread_p);

/* Forcibly, immidiately terminates `thread`. */
void p_mt_thread_terminate(p_mt_thread_t *thread_p);

/** MUTEXES **/
struct p_mt_mutex;
typedef struct p_mt_mutex * p_mt_mutex_t;

/* Returns a newly allocated mutex. Always succeeds. */
p_mt_mutex_t p_mt_mutex_create(void);

/* (Un)Locks `mutex`. */
void p_mt_mutex_lock(p_mt_mutex_t mutex);
void p_mt_mutex_unlock(p_mt_mutex_t mutex);

/* Deallocates `mutex`. */
void p_mt_mutex_destroy(p_mt_mutex_t *mutex_p);

#endif /* P_THREAD_H_ */
