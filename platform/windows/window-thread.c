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
#include <stdatomic.h>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif /* WIN32_LEAN_AND_MEAN */
#include <windows.h>
#include <errhandlingapi.h>

#define MODULE_NAME "window-thread"

struct window_thread_global_data {
    HWND win_handle;
    _Atomic bool running;
};

static i32 create_window(const char *title, u32 flags,
    const struct p_window_info *info,
    struct window_thread_global_data *global_data_p,
    HWND *o_win_handle, RECT *o_win_rect);
static void destroy_window(HWND *win_handle_p);

static void handle_thread_request(void *arg);

static LRESULT CALLBACK
window_procedure(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

void window_thread_fn(void *arg)
{
    struct window_init *init = arg;

    struct window_thread_global_data global_data = {
        .win_handle = NULL,
        .running = ATOMIC_VAR_INIT(false),
    };
    i32 result = 0;

    /* Create the window */
    p_mt_mutex_lock(&init->mutex);
    {
        result = create_window(
            init->in.title, init->in.flags, init->in.win_info,
            &global_data, &global_data.win_handle, &init->out.window_rect
        );

        init->out.win_handle = global_data.win_handle;
        init->out.running_p = &global_data.running;
        init->result = result;

        /* All of `global_data` already initialized */
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
    atomic_store(&global_data.running, true);
    while (atomic_load(&global_data.running)) {
        if (GetMessage(&msg, NULL, 0, 0) < 0)
            s_log_fatal("GetMessage() failed: %s", get_last_error_msg());

        DispatchMessage(&msg);
    }

    s_log_debug("Received quit request, starting cleanup...");
    destroy_window(&global_data.win_handle);

    s_log_debug("window cleanup OK, calling thread_exit");
    p_mt_thread_exit();
}

void window_thread_request_operation(struct window_thread_request *req)
{
    u_check_params(req != NULL
        && req->type > REQ_OP_MIN__ && req->type < REQ_OP_MAX__
        && req->request_mutex != P_MT_MUTEX_NULL
        && req->win_handle != NULL);

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
    enum window_thread_request_type type, void *arg, p_mt_mutex_t mutex)
{
    u_check_params(type >= 0 && type < REQ_OP_MAX__ && win_handle != NULL);

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
        req.win_handle = win_handle,
        req.type = type;
        req.arg = arg;
        req.completion_cond = cond;
        req.request_mutex = mutex;
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
    struct window_thread_global_data *global_data_p,
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
        NULL, NULL, /* Parent window handle and menu handle, useless here */
        g_instance_handle, /* hInstance */
        global_data_p /* Parameters passed to the `WM_CREATE` message */
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
    s_assert(req->request_mutex != P_MT_MUTEX_NULL,
        "The request status mutex is not initialized");
    p_mt_mutex_lock(&req->request_mutex);
    {
        s_assert(req->status == REQ_STATUS_PENDING,
            "Invalid value of the request status %d, (type: %d)",
            req->status, req->type);
    }
    p_mt_mutex_unlock(&req->request_mutex);

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
    p_mt_mutex_lock(&req->request_mutex);
    req->status = status;
    if (req->completion_cond != NULL)
        p_mt_cond_signal(req->completion_cond);
    p_mt_mutex_unlock(&req->request_mutex);
}

static LRESULT CALLBACK
window_procedure(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == WM_NCCREATE) {
        const CREATESTRUCT *create_struct = (void *)lParam;
        struct window_thread_global_data *const global_data_p =
            create_struct->lpCreateParams;

        SetLastError(0);
        (void) SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)global_data_p);
        s_assert(GetLastError() == 0,
            "SetWindowLongPtr failed: %s", get_last_error_msg());

        return TRUE;
    }

    if (uMsg > REQ_OP_MIN__ && uMsg < REQ_OP_MAX__) {
        handle_thread_request((void *)wParam);
        return 0;
    }

    struct window_thread_global_data *global_data_p =
        (void *)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    switch (uMsg) {
    case WM_CLOSE:
        p_event_send(&(struct p_event) { .type = P_EVENT_QUIT });
        return 0;
    case WM_ENTERMENULOOP:
    case WM_ENTERSIZEMOVE:
        s_assert(global_data_p != NULL,
            "The global data pointer must be initialized by this point");
        s_log_warn("in_sizemove -> true");
        break;
    case WM_EXITMENULOOP:
    case WM_EXITSIZEMOVE:
        s_assert(global_data_p != NULL,
            "The global data pointer must be initialized by this point");
        s_log_warn("in_sizemove -> false");
        break;
    case CGD_WM_EV_QUIT_:
        s_assert(global_data_p != NULL,
            "The global data pointer must be initialized by this point");
        atomic_store(&global_data_p->running, false);
        PostQuitMessage(0);
        return 0;
    default:
        break;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
