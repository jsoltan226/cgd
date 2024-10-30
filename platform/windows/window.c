#include "../window.h"
#include "../event.h"
#include "../thread.h"
#include <core/log.h>
#include <core/util.h>
#include <core/shapes.h>
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

#define MODULE_NAME "window"

static void * event_dispatcher_thread_fn(void *arg);

static LRESULT CALLBACK
window_procedure(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

struct p_window * p_window_open(const unsigned char *title,
    const rect_t *area, const u32 flags)
{
    struct p_window *win = calloc(1, sizeof(struct p_window));
    s_assert(win != NULL, "calloc() failed for struct window");

    s_assert(g_instance_handle != NULL,
        "Global instance handle not initialized!");

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

#define WINDOW_CLASS_NAME "cgd-window"
    const WNDCLASS window_class = {
        .hInstance = g_instance_handle,
        .lpszClassName = WINDOW_CLASS_NAME,
        .lpfnWndProc = window_procedure,
    };

    if (RegisterClass(&window_class) == 0)
        goto_error("Failed to register the window class: %s",
            get_last_error_msg());

    win->win = CreateWindowEx(0, WINDOW_CLASS_NAME, (const char *)title,
        WS_OVERLAPPEDWINDOW, rect_arg_expand(win->rect),
        NULL, NULL, g_instance_handle, (void *)&win->state_ro
    );
    if (win->win == 0)
        goto_error("Failed to create the window: %s", get_last_error_msg());

    ShowWindow(win->win, g_n_cmd_show);

    if (p_mt_thread_create(&win->event_dispatcher.thread,
        event_dispatcher_thread_fn, (void *)win, 0))
    {
        goto_error("Failed to create the event dispather thread.");
    }
    win->event_dispatcher.running = true;

    s_log_debug("%s() OK; window position: [%u, %u]; window dimensions: %ux%u",
        __func__, win->rect.x, win->rect.y, win->rect.w, win->rect.h);

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

    p_window_unbind_fb(win);

    if (win->event_dispatcher.running) {
        win->event_dispatcher.running = false;
        p_mt_thread_wait(&win->event_dispatcher.thread);
    }

    /* We can ignore any (highly unlikely to occur) errors here,
     * as the program will soon terminate anyway */
    (void) DestroyWindow(win->win);

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

static void * event_dispatcher_thread_fn(void *arg)
{
    struct p_window *win = arg;

    MSG msg = { 0 };
    while (win->event_dispatcher.running) {
        GetMessage(&msg, NULL, 0, 0);
        DispatchMessage(&msg);
    }

    p_mt_thread_exit(NULL);
}

static LRESULT CALLBACK
window_procedure(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    struct window_state_rw *state_rw =
        (struct window_state_rw *)((CREATESTRUCT *)lParam)->lpCreateParams;

    switch (uMsg) {
    case WM_CLOSE:
        atomic_store(&state_rw->test, false);
        p_event_send(&(struct p_event) { .type = P_EVENT_QUIT });
        return 0;
        break;
    default:
        atomic_store(&state_rw->test, true);
        break;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
