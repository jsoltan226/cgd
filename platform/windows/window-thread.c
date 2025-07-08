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

#define MODULE_NAME "window-thread"

static i32 do_window_init(struct p_window *win, const char *title);
static void do_window_cleanup(struct p_window *win);
static void handle_thread_request(void *arg);

static LRESULT CALLBACK
window_procedure(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

void window_thread_fn(void *arg)
{
    struct window_init *init = arg;
    struct p_window *win = init->win;

    const i32 result = do_window_init(init->win, init->in.title);

    /* Communicate the init result to the main thread */
    atomic_store(&init->result, result);
    p_mt_cond_signal(init->cond);

    /* Any use of the init struct at this point
     * (except the window pointer) is undefined behaviour */
    init = NULL;

    /* Exit if window init failed */
    if (result != 0)
        p_mt_thread_exit();

    /* The message loop */
    MSG msg = { 0 };
    while (msg.message != WM_APP + REQ_OP_QUIT_) {
        if (GetMessage(&msg, NULL, 0, 0) < 0) {
            s_log_fatal("GetMessage() failed: %s", get_last_error_msg());
        }
        if (msg.message >= WM_APP && msg.message <= 0xBFFF)
            handle_thread_request((void *)msg.wParam);
        else
            DispatchMessage(&msg);
    }

    do_window_cleanup(win);

    p_mt_thread_exit();
}

void window_thread_request_operation(struct window_thread_request *req)
{
    u_check_params(req != NULL && req->window != NULL
        && req->type >= 0 && req->type < REQ_OP_MAX__);

    atomic_store(&req->status, REQ_STATUS_NOT_STARTED);

    if (PostMessage(req->window, WM_APP + req->type,
            (unsigned long long)req, 0) == 0)
    {
        s_log_error("Failed to post message to the window: %s",
            get_last_error_msg());
        atomic_store(&req->status, REQ_STATUS_FAILURE);
        if (req->completion_cond != NULL)
            p_mt_cond_signal(req->completion_cond);
        return;
    }

    atomic_store(&req->status, REQ_STATUS_PENDING);
}

i32 window_thread_request_operation_and_wait(HWND win_handle,
    enum window_thread_request_type type, void *arg)
{
    u_check_params(win_handle != NULL && type >= 0 && type < REQ_OP_MAX__);

    p_mt_cond_t cond = p_mt_cond_create();
    p_mt_mutex_t mutex = p_mt_mutex_create();

    struct window_thread_request req = {
        .window = win_handle,
        .type = type,
        .completion_cond = cond,
        .arg = arg
    };

    p_mt_mutex_lock(&mutex);
    {
        window_thread_request_operation(&req);
        if (atomic_load(&req.status) == REQ_STATUS_PENDING)
            p_mt_cond_wait(cond, mutex);
    }
    p_mt_mutex_unlock(&mutex);

    return !(atomic_load(&req.status) == REQ_STATUS_SUCCESS);
}

static i32 do_window_init(struct p_window *win, const char *title)
{
    struct p_window_info *const info = &win->info;

    /* Normal style, but non-resizeable and non-maximizeable */
#define WINDOW_STYLE (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX)

    /* Adjust the client rect to actually be what we want */
    win->window_rect.left = info->client_area.x;
    win->window_rect.top = info->client_area.y;
    win->window_rect.right = info->client_area.x + info->client_area.w;
    win->window_rect.bottom = info->client_area.y + info->client_area.h;
    if (AdjustWindowRect(&win->window_rect, WINDOW_STYLE, false) == 0)
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
    win->win = CreateWindowExA(0, WINDOW_CLASS_NAME, title, WINDOW_STYLE,
        win->window_rect.left, /* x */
        win->window_rect.top, /* y */
        win->window_rect.right - win->window_rect.left, /* width */
        win->window_rect.bottom - win->window_rect.top, /* height */
        NULL, NULL, g_instance_handle, NULL
    );
    if (win->win == NULL)
        goto_error("Failed to create the window: %s", get_last_error_msg());

    /* Get the GDI device context */
    if (win->window_dc = GetDC(win->win), win->window_dc == NULL)
        goto_error("Failed to get the window DC: %s", get_last_error_msg());

    /* Make the window visible */
    (void) ShowWindow(win->win, g_n_cmd_show);

    return 0;

err:
    return 1;
}

static void do_window_cleanup(struct p_window *win)
{
    if (win->win != NULL) {
        /* Destroy the window */
        if (DestroyWindow(win->win) == 0) {
            s_log_error("Failed to destroy the window: %s",
                get_last_error_msg());
        }
        win->win = NULL;
    }
}

static void handle_thread_request(void *arg)
{
    struct window_thread_request *const req = arg;

    s_assert(req->status == REQ_STATUS_PENDING,
        "Invalid value of the request status");

    enum window_thread_request_status status = REQ_STATUS_PENDING;

    const union {
        struct render_init_software_req *render_init_software;
        struct render_present_software_req *render_present_software;
        struct render_destroy_software_req *render_destroy_software;
        void *voidp;
    } a = { .voidp = req->arg };

    switch (req->type) {
    case REQ_OP_RENDER_INIT_SOFTWARE:
        ;
        const i32 ret = render_init_software(
            a.render_init_software->ctx,
            a.render_init_software->win_info,
            a.render_init_software->win_handle
        );
        status = ret == 0 ? REQ_STATUS_SUCCESS : REQ_STATUS_FAILURE;
        break;
    case REQ_OP_RENDER_PRESENT_SOFTWARE:
        a.render_present_software->o_new_back_buf =
            render_present_software(a.render_present_software->ctx);
        status = a.render_present_software != NULL ?
            REQ_STATUS_SUCCESS : REQ_STATUS_FAILURE;
        break;
    case REQ_OP_RENDER_DESTROY_SOFTWARE:
        render_destroy_software(a.render_destroy_software->ctx);
        status = REQ_STATUS_SUCCESS;
        break;
    case REQ_OP_QUIT_:
    case REQ_OP_MAX__:
    default:
        s_log_fatal("Invalid request type %d", req->type);
    }

    s_log_trace("Handled request type %d with status %d",
        req->type, status);

    atomic_store(&req->status, status);
    if (req->completion_cond != NULL)
        p_mt_cond_signal(req->completion_cond);
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
