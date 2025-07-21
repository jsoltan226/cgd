#define P_INTERNAL_GUARD__
#include "window-thread.h"
#undef P_INTERNAL_GUARD__
#define P_INTERNAL_GUARD__
#include "window-internal.h"
#undef P_INTERNAL_GUARD__
#define P_INTERNAL_GUARD__
#include "window-present-sw.h"
#undef P_INTERNAL_GUARD__
#define P_INTERNAL_GUARD__
#include "error.h"
#undef P_INTERNAL_GUARD__
#define P_INTERNAL_GUARD__
#include "global.h"
#undef P_INTERNAL_GUARD__
#include "../event.h"
#include "../thread.h"
#include "../window.h"
#include <core/int.h>
#include <core/log.h>
#include <core/util.h>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif /* WIN32_LEAN_AND_MEAN */
#include <windows.h>

#define MODULE_NAME "window-thread"

static i32 create_window(const char *title, u32 flags,
    const struct p_window_info *info,
    HWND *o_win_handle, RECT *o_win_rect);
static void destroy_window(HWND *win_handle_p);
static void handle_thread_request(void *arg);

static LRESULT CALLBACK
window_procedure(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

void window_thread_fn(void *arg)
{
    struct window_init *init = arg;

    HWND win_handle = NULL;
    i32 result = 0;

    /* Create the window */
    p_mt_mutex_lock(&init->mutex);
    {
        result = create_window(
            init->in.title, init->in.flags, init->in.win_info,
            &win_handle, &init->out.window_rect
        );

        init->out.win_handle = win_handle;
        init->result = result;
    }
    p_mt_mutex_unlock(&init->mutex);

    /* Any use of the init struct at this point is undefined behaviour
     * because once the main thread gets signalled,
     * the underlying struct (that's constructed on the stack)
     * might go out of scope at any moment */
    p_mt_cond_t cond = init->cond;
    init = NULL;
    p_mt_cond_signal(cond);

    /* Exit if window init failed */
    if (result != 0)
        p_mt_thread_exit();

    /* The message loop */
    MSG msg = { 0 };
    i32 GM_ret = 0;
    while (
        (GM_ret = GetMessage(&msg, NULL, 0, 0), GM_ret) >= 0 &&
        (msg.message != CGD_WM_EV_QUIT_)
    ) {
        if (msg.message > REQ_OP_MIN__ && msg.message < REQ_OP_MAX__)
            handle_thread_request((void *)msg.wParam);
        else
            DispatchMessage(&msg);
    }
    if (GM_ret < 0)
        s_log_fatal("GetMessage() failed: %s", get_last_error_msg());

    s_log_debug("Received quit request, starting cleanup...");
    destroy_window(&win_handle);

    s_log_debug("window cleanup OK, calling thread_exit");
    p_mt_thread_exit();
}

void window_thread_request_operation(struct window_thread_request *req)
{
    u_check_params(req != NULL && req->win_handle != NULL
        && req->type > REQ_OP_MIN__ && req->type < REQ_OP_MAX__
        && req->status_mutex != P_MT_MUTEX_NULL);

    req->status = REQ_STATUS_NOT_STARTED;

    if (PostMessage(req->win_handle, req->type,
            (unsigned long long)req, 0) == 0)
    {
        s_log_error("Failed to post message to the window: %s",
            get_last_error_msg());
        req->status = REQ_STATUS_FAILURE;
        return;
    }

   req->status = REQ_STATUS_PENDING;
}

i32 window_thread_request_operation_and_wait(HWND win_handle,
    enum window_thread_request_type type, p_mt_mutex_t mutex, void *arg
)
{
    u_check_params(type >= 0 && type < REQ_OP_MAX__);

    p_mt_cond_t cond = p_mt_cond_create();
    p_mt_mutex_t tmp_mutex_ = P_MT_MUTEX_NULL;
    if (mutex == P_MT_MUTEX_NULL) {
        tmp_mutex_ = p_mt_mutex_create();
        mutex = tmp_mutex_;
    }

    enum window_thread_request_status status;
    struct window_thread_request req;

    p_mt_mutex_lock(&mutex);
    {
        req.win_handle = win_handle;
        req.type = type;
        req.completion_cond = cond;
        req.arg = arg;
        req.status_mutex = mutex;
        req.status = REQ_STATUS_NOT_STARTED;

        window_thread_request_operation(&req);
        while (req.status == REQ_STATUS_PENDING)
            p_mt_cond_wait(cond, mutex);

        status = req.status;
    }
    p_mt_mutex_unlock(&mutex);

    if (tmp_mutex_ != P_MT_MUTEX_NULL)
        p_mt_mutex_destroy(&tmp_mutex_);

    return !(status == REQ_STATUS_SUCCESS);
}

static i32 create_window(const char *title, u32 flags,
    const struct p_window_info *info,
    HWND *o_win_handle, RECT *o_win_rect)
{
    (void) flags;

    /* Normal style, but non-resizeable and non-maximizeable */
#define WINDOW_STYLE (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX)

    /* Adjust the client rect to actually be what we want */
    o_win_rect->left = info->client_area.x;
    o_win_rect->top = info->client_area.y;
    o_win_rect->right = info->client_area.x + info->client_area.w;
    o_win_rect->bottom = info->client_area.y + info->client_area.h;
    if (AdjustWindowRect(o_win_rect, WINDOW_STYLE, false) == 0)
        goto_error("Failed to adjust the window rect: %s", get_last_error_msg());

    /* Register the window class */
#define WINDOW_CLASS_NAME "cgd-window"
    const WNDCLASS window_class = {
        .hInstance = g_instance_handle,
        .lpszClassName = WINDOW_CLASS_NAME,
        .lpfnWndProc = window_procedure,
        .hCursor = LoadCursor(NULL, IDC_ARROW),
        .hIcon = LoadIcon(NULL, IDI_APPLICATION),
    };

    if (RegisterClass(&window_class) == 0)
        goto_error("Failed to register the window class: %s",
            get_last_error_msg());

    /* Create the window */
    *o_win_handle = CreateWindowExA(0, WINDOW_CLASS_NAME, title, WINDOW_STYLE,
        o_win_rect->left, /* x */
        o_win_rect->top, /* y */
        o_win_rect->right - o_win_rect->left, /* width */
        o_win_rect->bottom - o_win_rect->top, /* height */
        NULL, NULL, g_instance_handle, NULL
    );
    if (*o_win_handle == NULL)
        goto_error("Failed to create the window: %s", get_last_error_msg());

    /* Make the window visible */
    (void) ShowWindow(*o_win_handle, g_n_cmd_show);

    return 0;

err:
    return 1;
}

static void destroy_window(HWND *win_handle_p)
{
    if (*win_handle_p != NULL) {
        if (DestroyWindow(*win_handle_p) == 0) {
            s_log_error("Failed to destroy the window: %s",
                get_last_error_msg());
        }
        *win_handle_p = NULL;
    }
}

static void handle_thread_request(void *arg)
{
    struct window_thread_request *const req = arg;

    /* Check the arg */
    s_assert(req != NULL, "The request is NULL");
    s_assert(req->type > REQ_OP_MIN__ && req->type < REQ_OP_MAX__,
        "Invalid request type %d", req->type);
    s_assert(req->status_mutex != P_MT_MUTEX_NULL,
        "The request status mutex is not initialized");
    p_mt_mutex_lock(&req->status_mutex);
    {
        s_assert(req->status == REQ_STATUS_PENDING,
            "Invalid value of the request status %d, (type: %d)",
            req->status, req->type);
    }
    p_mt_mutex_unlock(&req->status_mutex);

    enum window_thread_request_status status;

    /* Actually handle the request */
    switch (req->type) {
    case REQ_OP_RENDER_INIT_SOFTWARE:
    case REQ_OP_RENDER_PRESENT_SOFTWARE:
    case REQ_OP_RENDER_DESTROY_SOFTWARE:
        status =
            render_software_handle_window_thread_request(req->type, req->arg);
        break;
    case REQ_OP_MIN__:
    case REQ_OP_MAX__:
    default:
        s_log_fatal("impossible outcome");
    }

    s_log_trace("Handled request type %d with status %d",
        req->type, status);

    /* Signal completion to the requester */
    p_mt_mutex_lock(&req->status_mutex);
    req->status = status;
    if (req->completion_cond != NULL)
        p_mt_cond_signal(req->completion_cond);
    p_mt_mutex_unlock(&req->status_mutex);
}

static LRESULT CALLBACK
window_procedure(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
    case WM_CLOSE:
        p_event_send(&(struct p_event) { .type = P_EVENT_QUIT });
        break;
    default:
        break;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
