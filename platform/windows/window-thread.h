#ifndef WINDOW_THREAD_H_
#define WINDOW_THREAD_H_

#include <platform/common/guard.h>

#include "../thread.h"
#include <core/int.h>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif /* WIN32_LEAN_AND_MEAN */
#include <windef.h>
#include <windows.h>

/* This header file defines the API around the window thread -
 * the thread that owns and manages the window and its related objects.
 * Due to the way the win32 API works, some functions can only be executed
 * from the thread that owns the window (or in general, a given object).
 *
 * Because of this it would be very troublesome for a mulithreaded program
 * to use the `p_window` API, and so a solution was made -
 * a separate thread manages these operation, and it can be asked to do them
 * for you.
 *
 * Here is an example of how this works:
 * You have a function, e.g. `render_init_software`
 * that needs to be executed from the window thread.
 * And so, a request ID is assigned to it, and any thread can send
 * it with the below function(s), while passing in the appropriate arguments.
 * The window thread, after receiving the request, will check
 * which module implements that operation.
 *
 * The modules that have such functions  all export a handler function for all of
 * their relevant request types, and that handler then calls
 * the real function that implements the request.
 *
 * After it's done, it returns to the window thread main function,
 * which then informs the original requester about the status of the operation.
 *
 * So again, to illustrate:
 *
 * CALLER THREAD:                               WINDOW THREAD:
 *  `p_window_set_acceleration`                     Idle (`window_thread_fn`)
 *  `window_thread_request_operation_and_wait`      Idle
 *  `window_thread_request_operation`    ===>       `GetMessage`
 *  Idle                                            `handle_thread_request`
 *  Idle                                            `render_software_handle_-
 *                                                      window_thread_request`
 *  Idle                                            `do_render_init_software`
 *  Idle                                            return to `window_thread_fn`
 *  `p_mt_cond_wait`                     <===       `p_mt_cond_signal`
 *  return to `p_window_set_acceleration`           Idle
 *      and check results
 *
 * Kinda like a syscall lol */

/* The thread function that `p_window_open` creates the window thread with.
 * The argument is the `struct window_init` from "window-internal.h". */
extern void window_thread_fn(void *arg);

/* The event ID used to signal the window thread to quit */
#define CGD_WM_EV_QUIT_ (WM_APP)

/* All the possible window thread request IDs.
 * Consult the modules that implement them for more details.
 * Note that `REQ_OP_MIN__` and `REQ_OP_MAX__` are not valid request IDs! */
enum window_thread_request_type {
    /* `WM_APP` is reserved for `CGD_WM_EV_QUIT_` */
    REQ_OP_MIN__ = WM_APP + 1,

    /* Implemented by `platform/windows/window-present-sw` */
    REQ_OP_RENDER_INIT_SOFTWARE             = WM_APP + 2,
    REQ_OP_RENDER_PRESENT_SOFTWARE          = WM_APP + 3,
    REQ_OP_RENDER_DESTROY_SOFTWARE          = WM_APP + 4,

    REQ_OP_MAX__
};

/* The possible statuses of a window thread request
 * at any given moment.
 * It starts of as `NOT_STARTED`, then after being sent
 * to the thread it's `PENDING`, and when done,
 * the thread sets the final result - either a `SUCCESS` or a `FAILURE`. */
enum window_thread_request_status {
    REQ_STATUS_NOT_STARTED,
    REQ_STATUS_PENDING,
    REQ_STATUS_SUCCESS,
    REQ_STATUS_FAILURE
};

struct window_thread_request {
    /* The handle to the window to which the request will be sent */
    HWND win_handle;

    /* The type (ID) of the request */
    enum window_thread_request_type type;

    /* The request-specific argument.
     * For definitions of which requests take what kinds of arguments,
     * refer to the individual files that implement those requests. */
    void *arg;

    /* The status of the request.
     * Only relevant as a "return" value
     * (doesn't have to be initialized by the requester) */
    enum window_thread_request_status status;

    /* The mutex that protects the status variable
     * and (if used) the condition variable */
    p_mt_mutex_t request_mutex;

    /* The condition variable that will be signalled once the request
     * is fulfilled. This parameter is optional -
     * if you don't want to use this, just set it to `NULL`. */
    p_mt_cond_t completion_cond;
};

/* The lower-level function that sends the request to the window thread.
 * It's very prone to race conditions, so please carefully read the following:
 *
 * `req` must be initialized as follows:
 *  - `win_handle` must be a valid handle to the window
 *  - `type` must be a valid request ID (see `enum window_thread_request_type`)
 *  - `arg` should be set to whatever your specific request needs
 *  - `status_mutex` must be a valid, LOCKED mutex!!!
 *  - `completion_cond` is optional
 *
 *  The struct must be initialized (written to) while `status_mutex` is locked!
 */
void window_thread_request_operation(struct window_thread_request *req);

/* The higher-level (preferred) function that sends a request
 * to the window thread and waits for it's completion.
 *
 * Returns 0 if the request succeeds and non-zero if it fails.
 *
 *  - `win_handle` is the window to which the request will be sent
 *  - `type` must be a valid request ID (see `enum window_thread_request_type`)
 *  - `arg` should be set to whatever your specific request needs
 *  - `mutex` is optional - it's the mutex that will be used to wait
 *      on the request cond (passed in as `status_mutex`).
 *      Can be `NULL`; in that case the function will create a temporary one
 *      and use it instead.
 *      You should pass in your own one if you
 *      reuse it between requests - for optimisation.
 */
i32 window_thread_request_operation_and_wait(HWND win_handle,
    enum window_thread_request_type type, void *arg, p_mt_mutex_t mutex);

#endif /* WINDOW_THREAD_H_ */
