#include "../window.h"
#include "../event.h"
#include "../thread.h"
#include <core/log.h>
#include <core/util.h>
#include <core/shapes.h>
#include <malloc.h>
#include <processthreadsapi.h>
#include <stdlib.h>
#include <string.h>
#include <stdatomic.h>
#ifndef WIN32_LEAN_AND_MEAN
#define WINWIN32_LEAN_AND_MEAN
#endif /* WIN32_LEAN_AND_MEAN */
#include <windows.h>
#include <wingdi.h>
#include <minwindef.h>
#define P_INTERNAL_GUARD__
#include "window-internal.h"
#undef P_INTERNAL_GUARD__
#define P_INTERNAL_GUARD__
#include "error.h"
#undef P_INTERNAL_GUARD__
#define P_INTERNAL_GUARD__
#include "global.h"
#undef P_INTERNAL_GUARD__

#define CGD_LOCAL_QUIT_INTERRUPT_EVENT (WM_APP + 1)

#define MODULE_NAME "window"

static void * thread_fn(void *arg);
static i32 do_window_init(struct p_window *win,
    const char *title, const rect_t *area, const u32 flags);

static LRESULT CALLBACK
window_procedure(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

struct p_window * p_window_open(const unsigned char *title,
    const rect_t *area, const u32 flags)
{
    struct p_window *win = calloc(1, sizeof(struct p_window));
    s_assert(win != NULL, "calloc() failed for struct window");

    s_assert(g_instance_handle != NULL,
        "Global instance handle not initialized!");

    struct window_init init = {
        /* The p_window structure to be initialized */
        .win = win,

        /* PARAMS */
        .title = (const char *)title,
        .area = area,
        .flags = flags,

        /* Temporary thread sync/communication objects */
        .cond = p_mt_cond_create(),
        .mutex = p_mt_mutex_create(),

        .result = 0
    };

    /* The mutex must be locked before calling `p_mt_cond_wait` */
    p_mt_mutex_lock(&init.mutex);

    /* Create the thread */
    if (p_mt_thread_create(&win->thread, thread_fn, &init, 0)) {
        p_mt_cond_destroy(&init.cond);
        p_mt_mutex_destroy(&init.mutex);
        goto_error("Failed to spawn window thread");
    }

    /* Wait for the thread to complete window initialization */
    p_mt_cond_wait(init.cond, init.mutex);

    /* Clean up temporary objects */
    p_mt_cond_destroy(&init.cond);
    p_mt_mutex_destroy(&init.mutex);

    /* Check the results */
    if (init.result != 0)
        goto_error("Window init failed.");

    win->initialized = true;
    s_log_debug("%s() OK; window position: [%u, %u]; window dimensions: %ux%u",
        __func__, win->rect.x, win->rect.y, win->rect.w, win->rect.h);

    /* Initialize the event queue */
    p_event_send(&(const struct p_event) { .type = P_EVENT_CTL_INIT_ });

    return win;

err:
    p_window_close(&win);
    return NULL;
}

void p_window_bind_fb(struct p_window *win, struct pixel_flat_data *fb)
{
    u_check_params(win != NULL && fb != NULL);

    s_assert(win->bound_fb == NULL,
        "Another framebuffer is already bound to this window.");
    s_assert(win->rect.w == fb->w && win->rect.h == fb->h,
        "Attempt to bind a framebuffer of invalid size "
        "(size of provided fb is %ux%u, should be %ux%u)",
        fb->w, fb->h, win->rect.w, win->rect.h);

    win->bound_fb = fb;
}

void p_window_unbind_fb(struct p_window *win)
{
    u_check_params(win != NULL);
    win->bound_fb = NULL;
}

void p_window_render(struct p_window *win)
{
    u_check_params(win != NULL);
    if (win->bound_fb == NULL) return;
}

void p_window_close(struct p_window **win_p)
{
    if (win_p == NULL || *win_p == NULL) return;
    struct p_window *win = *win_p;

    s_log_debug("Destroying window...");

    /* Just for good measure */
    p_window_unbind_fb(win);

    const DWORD thread_id = GetThreadId(win->thread);

    if (!win->initialized) {
        /* Window init failed
         * (the thread should have already exited by itself) */
        p_mt_thread_wait(&win->thread);
    } else if (PostThreadMessage(thread_id, CGD_LOCAL_QUIT_INTERRUPT_EVENT, 0, 0) == 0) {
        /* The window is OK, but we could't send it the message
         * to exit gracefully by itself, so we have reach out for
         * more drastic measures (or else the execution will deadlock here) */
        s_log_error("Failed to send the quit interrupt event "
                "to the listener thread: %s", get_last_error_msg());
        p_mt_thread_terminate(&win->thread);
    } else {
        /* The normal case (window is OK and we
         * successfully sent it the termination message) */
        p_mt_thread_wait(&win->thread);
    }

    /* We can ignore any (highly unlikely to occur) errors here,
     * since the program will soon terminate anyway */
    if (win->win != 0)
        (void) DestroyWindow(win->win);

    /* Destroy the event queue */
    p_event_send(&(const struct p_event) { .type = P_EVENT_CTL_DESTROY_ });

    u_nzfree(win_p);
}

i32 p_window_get_meta(const struct p_window *win, struct p_window_meta *out)
{
    if (win == NULL || out == NULL)
        return -1;

    out->x = win->rect.x;
    out->y = win->rect.y;
    out->w = win->rect.w;
    out->h = win->rect.h;
    out->color_type = P_WINDOW_RGBA8888;

    return 0;
}

static void * thread_fn(void *arg)
{
    struct window_init *init = arg;

    const i32 result = do_window_init(init->win,
        init->title, init->area, init->flags);

    /* Communicate the init result to the main thread */
    init->result = result;
    p_mt_cond_signal(init->cond);

    /* Any use of the init struct at this point
     * (except the window pointer) is undefined behaviour */
    init = NULL;

    /* Exit if window init failed */
    if (result != 0)
        p_mt_thread_exit(NULL);

    /* The message loop */
    MSG msg = { 0 };
    while (msg.message != CGD_LOCAL_QUIT_INTERRUPT_EVENT) {
        if (GetMessage(&msg, NULL, 0, 0) < 0) {
            s_log_fatal(MODULE_NAME, __func__,
                "GetMessage() failed: %s", get_last_error_msg());
        }
        DispatchMessage(&msg);
    }

    p_mt_thread_exit(NULL);
}

static i32 do_window_init(struct p_window *win,
    const char *title, const rect_t *area, const u32 flags)
{
    /* Get screen resolution */
    win->screen_w = GetSystemMetrics(SM_CXSCREEN);
    if (win->screen_w == 0)
        goto_error("Failed to get screen width: %s", get_last_error_msg());

    win->screen_h = GetSystemMetrics(SM_CYSCREEN);
    if (win->screen_h == 0)
        goto_error("Failed to get screen height: %s", get_last_error_msg());

    /* Handle P_WINDOW_POS_CENTERED flags */
    win->rect.x = area->x;
    if (flags & P_WINDOW_POS_CENTERED_X)
        win->rect.x = (win->screen_w - area->w) / 2;

    win->rect.y = area->y;
    if (flags & P_WINDOW_POS_CENTERED_Y)
        win->rect.y = (win->screen_h - area->h) / 2;

    win->rect.w = area->w;
    win->rect.h = area->h;

    /* Register the window class */
#define WINDOW_CLASS_NAME "cgd-window"
    const WNDCLASS window_class = {
        .hInstance = g_instance_handle,
        .lpszClassName = WINDOW_CLASS_NAME,
        .lpfnWndProc = window_procedure,
    };

    if (RegisterClass(&window_class) == 0)
        goto_error("Failed to register the window class: %s",
            get_last_error_msg());

    /* Create the window */
    win->win = CreateWindowEx(0, WINDOW_CLASS_NAME, title,
        WS_OVERLAPPEDWINDOW, rect_arg_expand(win->rect),
        NULL, NULL, g_instance_handle, NULL
    );
    if (win->win == 0)
        goto_error("Failed to create the window: %s", get_last_error_msg());

    /* Make the window visible */
    (void) ShowWindow(win->win, g_n_cmd_show);

    return 0;

err:
    return 1;
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
