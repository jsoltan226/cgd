#include "../window.h"
#include "../event.h"
#include "../thread.h"
#include <core/log.h>
#include <core/util.h>
#include <core/pixel.h>
#include <core/shapes.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <stdatomic.h>
#ifndef WIN32_LEAN_AND_MEAN
#define WINWIN32_LEAN_AND_MEAN
#endif /* WIN32_LEAN_AND_MEAN */
#include <windows.h>
#include <winnt.h>
#include <wingdi.h>
#include <winuser.h>
#include <minwindef.h>
#include <processthreadsapi.h>
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

static void thread_fn(void *arg);
static i32 do_window_init(struct p_window *win,
    const char *title, const rect_t *area, const u32 flags);
static void do_window_cleanup(struct p_window *win);

static LRESULT CALLBACK
window_procedure(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

struct p_window * p_window_open(const char *title,
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
    if (p_mt_thread_create(&win->thread, thread_fn, &init)) {
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
    s_log_verbose("%s() OK; window position: [%i, %i]; window dimensions: %ux%u",
        __func__, rect_arg_expand(win->info.client_area));

    /* Initialize the event queue */
    p_event_send(&(const struct p_event) { .type = P_EVENT_CTL_INIT_ });

    return win;

err:
    p_window_close(&win);
    return NULL;
}

void p_window_render(struct p_window *win, struct pixel_flat_data *fb)
{
    u_check_params(win != NULL && fb != NULL);

    s_assert(win->info.client_area.w == fb->w
        && win->info.client_area.h == fb->h,
        "Attempt to render a framebuffer of invalid size "
        "(size of provided fb is %ux%u, should be %ux%u)",
        fb->w, fb->h, win->info.client_area.w, win->info.client_area.h);

    const BITMAPINFO bmi = {
        .bmiHeader = {
            .biSize = sizeof(BITMAPINFO),
            .biWidth = fb->w,
            .biHeight = -fb->h,
            .biPlanes = 1,
            .biBitCount = 32,
            .biCompression = BI_RGB
        },
    };
#define DST_X 0
#define DST_Y 0
#define SRC_X 0
#define SRC_Y 0
#define WIDTH fb->w
#define HEIGHT fb->h
#define START_SCANLINE 0
#define N_LINES HEIGHT
    (void) SetDIBitsToDevice(win->dc, DST_X, DST_Y, WIDTH, HEIGHT,
        SRC_X, SRC_Y, START_SCANLINE, N_LINES,
        fb->buf, &bmi, DIB_RGB_COLORS);
}

void p_window_close(struct p_window **win_p)
{
    if (win_p == NULL || *win_p == NULL) return;
    struct p_window *win = *win_p;

    s_log_verbose("Destroying window...");

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

void p_window_get_info(const struct p_window *win, struct p_window_info *out)
{
    u_check_params(win != NULL && out != NULL);
    memcpy(out, &win->info, sizeof(struct p_window_info));
}

static void thread_fn(void *arg)
{
    struct window_init *init = arg;
    struct p_window *win = init->win;

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
        p_mt_thread_exit();

    /* The message loop */
    MSG msg = { 0 };
    while (msg.message != CGD_LOCAL_QUIT_INTERRUPT_EVENT) {
        if (GetMessage(&msg, NULL, 0, 0) < 0) {
            s_log_fatal("GetMessage() failed: %s", get_last_error_msg());
        }
        DispatchMessage(&msg);
    }

    do_window_cleanup(win);

    p_mt_thread_exit();
}

static i32 do_window_init(struct p_window *win,
    const char *title, const rect_t *area, const u32 flags)
{
    struct p_window_info *const info = &win->info;

    /* Get screen resolution */
    info->display_rect.w = GetSystemMetrics(SM_CXSCREEN);
    if (info->display_rect.w == 0)
        goto_error("Failed to get screen width: %s", get_last_error_msg());

    info->display_rect.h = GetSystemMetrics(SM_CYSCREEN);
    if (info->display_rect.h == 0)
        goto_error("Failed to get screen height: %s", get_last_error_msg());

    /* Handle P_WINDOW_POS_CENTERED flags */
    info->client_area.x = area->x;
    if (flags & P_WINDOW_POS_CENTERED_X)
        info->client_area.x = (info->display_rect.w - area->w) / 2;

    info->client_area.y = area->y;
    if (flags & P_WINDOW_POS_CENTERED_Y)
        info->client_area.y = (info->display_rect.h - area->h) / 2;

    info->client_area.w = area->w;
    info->client_area.h = area->h;

    /* Initialize the rest of the info struct */
    info->display_color_format = BGRX32;
    info->gpu_acceleration = P_WINDOW_ACCELERATION_NONE;
    info->vsync_supported = false;

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
    if (win->win == 0)
        goto_error("Failed to create the window: %s", get_last_error_msg());

    /* Get the device context of the window */
    win->dc = GetDC(win->win);
    if (win->dc == NULL)
        goto_error("Failed to get the window device context handle: %s",
            get_last_error_msg());

    /* Make the window visible */
    (void) ShowWindow(win->win, g_n_cmd_show);

    return 0;

err:
    return 1;
}

static void do_window_cleanup(struct p_window *win)
{
    /* Release the window's device context */
    (void) ReleaseDC(win->win, win->dc);
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
