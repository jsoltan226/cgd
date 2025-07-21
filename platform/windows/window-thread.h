#ifndef WINDOW_THREAD_H_
#define WINDOW_THREAD_H_

#include "../thread.h"
#include <core/int.h>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif /* WIN32_LEAN_AND_MEAN */
#include <windef.h>
#include <windows.h>

#ifndef P_INTERNAL_GUARD__
#error This header file is internal to the cgd platform module and is not intended to be used elsewhere
#endif /* P_INTERNAL_GUARD__ */

extern void window_thread_fn(void *arg);

/* The event ID used to signal the window thread to quit */
#define CGD_WM_EV_QUIT_ (WM_APP)

enum window_thread_request_type {
    /* Reserved for `CGD_WM_EV_QUIT_` */
    REQ_OP_MIN__ = WM_APP,

    /* Implemented by `platform/windows/window-present-sw` */
    REQ_OP_RENDER_INIT_SOFTWARE             = WM_APP + 1,
    REQ_OP_RENDER_PRESENT_SOFTWARE          = WM_APP + 2,
    REQ_OP_RENDER_DESTROY_SOFTWARE          = WM_APP + 3,

    REQ_OP_MAX__
};

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
    i32 status;

    /* The mutex that protects the status variable
     * and (if used) the condition variable */
    p_mt_mutex_t status_mutex;

    /* The condition variable that will be signalled once the request
     * is fulfilled. This parameter is optional -
     * if you don't want to use this, just set it to `NULL`. */
    p_mt_cond_t completion_cond;
};
void window_thread_request_operation(struct window_thread_request *req);

i32 window_thread_request_operation_and_wait(HWND win_handle,
    enum window_thread_request_type type, p_mt_mutex_t mutex, void *arg
);

#endif /* WINDOW_THREAD_H_ */
