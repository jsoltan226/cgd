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
static i32 do_window_init(struct p_window *win, const char *title);
static void do_window_cleanup(struct p_window *win);

static i32 check_flags(const u32 flags);
static void set_default_flags(u32 *flags);
static i32 init_window_info(struct p_window_info *info,
    const rect_t *area, u32 flags);

static i32 initialize_acceleration(struct p_window *win,
    const enum p_window_flags flags);

static i32 render_init_software(struct window_render_sw_ctx *sw_rctx,
    const struct p_window *win);
static struct pixel_flat_data * render_present_software(
    struct window_render_sw_ctx *sw_rctx,
    enum p_window_present_mode present_mode
);
static void render_destroy_software(struct window_render_sw_ctx *sw_rctx,
    const struct p_window *win);

static LRESULT CALLBACK
window_procedure(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

struct p_window * p_window_open(const char *title,
    const rect_t *area, u32 flags)
{
    u_check_params(area != NULL && check_flags(flags) == 0);
    set_default_flags(&flags);

    struct p_window *win = calloc(1, sizeof(struct p_window));
    s_assert(win != NULL, "calloc() failed for struct window");

    s_assert(g_instance_handle != NULL,
        "Global instance handle not initialized!");

    memset(win, 0, sizeof(struct p_window));

    /* Populate the info struct */
    if (init_window_info(&win->info, area, flags))
        goto_error("Failed to init the window info struct");

    /* Create the window thread, which will do the rest of the work */
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

    /* Initialize anything acceleration-specific */
    if (initialize_acceleration(win, flags))
        goto_error("Failed to initialize window acceleration");

    /* Initialize the event queue */
    p_event_send(&(const struct p_event) { .type = P_EVENT_CTL_INIT_ });

    return win;

err:
    p_window_close(&win);
    return NULL;
}

struct pixel_flat_data * p_window_swap_buffers(struct p_window *win,
    const enum p_window_present_mode present_mode)
{
    u_check_params(win != NULL);

    switch (win->info.gpu_acceleration) {
    case P_WINDOW_ACCELERATION_NONE:
        return render_present_software(&win->render.sw, present_mode);
    case P_WINDOW_ACCELERATION_OPENGL:
    case P_WINDOW_ACCELERATION_VULKAN:
        s_log_error("Vulkan & OpenGL acceleration aren't yet implemented "
            "on the Windows platform.");
        return NULL;
    case P_WINDOW_ACCELERATION_UNSET_:
        s_log_error("Attempt to swap buffers "
            "of a window with unset acceleration");
        return NULL;
    default:
        s_log_fatal("Invalid window acceleration mode (%d)",
            win->info.gpu_acceleration);
    }
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

    /* The window should already be destroyed by now,
     * but if it isn't, at least try to close it */
    if (win->win != NULL) {
        s_log_error("Window was not closed by the window thread");
        (void) DestroyWindow(win->win);
        win->win = NULL;
    }

    p_window_set_acceleration(win, P_WINDOW_ACCELERATION_UNSET_);

    /* Destroy the event queue */
    p_event_send(&(const struct p_event) { .type = P_EVENT_CTL_DESTROY_ });

    u_nzfree(win_p);
}

void p_window_get_info(const struct p_window *win, struct p_window_info *out)
{
    u_check_params(win != NULL && out != NULL);
    memcpy(out, &win->info, sizeof(struct p_window_info));
}

i32 p_window_set_acceleration(struct p_window *win,
    enum p_window_acceleration new_acceleration_mode)
{
    u_check_params(win != NULL);

    const enum p_window_acceleration old_acceleration =
        win->info.gpu_acceleration;
    win->win->info.gpu_acceleration = P_WINDOW_ACCELERATION_UNSET_;

    switch (old_acceleration) {
    case P_WINDOW_ACCELERATION_UNSET_:
        break;
    case P_WINDOW_ACCELERATION_NONE:
        if (new_acceleration_mode == P_WINDOW_ACCELERATION_NONE) {
            s_log_warn("Attempt to set window acceleration to software, "
                "but it's already enabled. Not doing anything.");
            return 0;
        }
        render_destroy_software(&win->render.sw, win);
        break;
    case P_WINDOW_ACCELERATION_OPENGL:
    case P_WINDOW_ACCELERATION_VULKAN:
        s_log_error("Vulkan & OpenGL acceleration aren't yet implemented "
            "on the Windows platform.");
        /* Fall-through */
    default:
        s_log_fatal("Invalid old window acceleration mode (%#x)",
            win->info.gpu_acceleration);
    }

    switch (new_acceleration_mode) {
    case P_WINDOW_ACCELERATION_UNSET_:
        break;
    case P_WINDOW_ACCELERATION_NONE:
        if (render_init_software(&win->render.sw, win)) {
            s_log_error("Failed to initialize software rendering");
            return 1;
        }
        break;
    case P_WINDOW_ACCELERATION_OPENGL:
    case P_WINDOW_ACCELERATION_VULKAN:
        s_log_error("Vulkan & OpenGL acceleration aren't yet implemented "
            "on the Windows platform.");
        return 1;
    default:
        s_log_fatal("Invalid new acceleration mode (%#x)",
            new_acceleration_mode);
    }

    win->info.gpu_acceleration = new_acceleration_mode;

    return 0;
}

static void thread_fn(void *arg)
{
    struct window_init *init = arg;
    struct p_window *win = init->win;

    const i32 result = do_window_init(init->win, init->title);

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

static void set_default_flags(u32 *flags)
{
    const u32 orig_flags = *flags;
#define not_set(flag) (!(orig_flags & flag))

    if (not_set(P_WINDOW_TYPE_NORMAL) && not_set(P_WINDOW_TYPE_DUMMY))
        *flags |= P_WINDOW_TYPE_NORMAL;

    if (not_set(P_WINDOW_VSYNC_SUPPORT_REQUIRED) &&
        not_set(P_WINDOW_VSYNC_SUPPORT_OPTIONAL))
    {
        *flags |= P_WINDOW_VSYNC_SUPPORT_OPTIONAL;
    }

    if (not_set(P_WINDOW_PREFER_ACCELERATED) &&
        not_set(P_WINDOW_REQUIRE_ACCELERATED) &&
        not_set(P_WINDOW_REQUIRE_OPENGL) &&
        not_set(P_WINDOW_REQUIRE_VULKAN) &&
        not_set(P_WINDOW_NO_ACCELERATION))
    {
        *flags |= P_WINDOW_PREFER_ACCELERATED;
    }

#undef not_set

}

static i32 check_flags(const u32 flags)
{
    static const enum p_window_flags contradicting_pairs[][2] = {
        { P_WINDOW_TYPE_NORMAL,  P_WINDOW_TYPE_DUMMY },
        { P_WINDOW_TYPE_DUMMY, P_WINDOW_REQUIRE_ACCELERATED },
        { P_WINDOW_TYPE_DUMMY, P_WINDOW_REQUIRE_VULKAN },
        { P_WINDOW_TYPE_DUMMY, P_WINDOW_REQUIRE_OPENGL },
        { P_WINDOW_PREFER_ACCELERATED, P_WINDOW_REQUIRE_ACCELERATED },
        { P_WINDOW_PREFER_ACCELERATED, P_WINDOW_REQUIRE_OPENGL },
        { P_WINDOW_PREFER_ACCELERATED, P_WINDOW_REQUIRE_VULKAN },
        { P_WINDOW_REQUIRE_ACCELERATED, P_WINDOW_REQUIRE_VULKAN },
        { P_WINDOW_REQUIRE_ACCELERATED, P_WINDOW_REQUIRE_OPENGL },
        { P_WINDOW_REQUIRE_VULKAN, P_WINDOW_REQUIRE_OPENGL },
        { P_WINDOW_VSYNC_SUPPORT_REQUIRED, P_WINDOW_VSYNC_SUPPORT_OPTIONAL },
    };
    i32 ret = 0;
    for (u32 i = 0; i < u_arr_size(contradicting_pairs); i++) {
        const u32 flag_a = contradicting_pairs[i][0];
        const u32 flag_b = contradicting_pairs[i][1];
        if (flags & flag_a && flags & flag_b) {
            i32 index_a = 0;
            i32 index_b = 0;
            while (flag_a != 1U << index_a) index_a++;
            while (flag_b != 1U << index_b) index_b++;

            s_log_error("Mutually exclusive flags: \"%s\" and \"%s\"",
                p_window_flag_strings[index_a], p_window_flag_strings[index_b]);
            ret++;
        }
    }

    return ret;

}

static i32 init_window_info(struct p_window_info *info,
    const rect_t *area, u32 flags)
{
    /* Get screen resolution */
    info->display_rect.w = GetSystemMetrics(SM_CXSCREEN);
    if (info->display_rect.w == 0) {
        s_log_error("Failed to get screen width: %s", get_last_error_msg());
        return 1;
    }

    info->display_rect.h = GetSystemMetrics(SM_CYSCREEN);
    if (info->display_rect.h == 0) {
        s_log_error("Failed to get screen height: %s", get_last_error_msg());
        return 1;
    }

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

    return 0;
}

static i32 initialize_acceleration(struct p_window *win,
    const enum p_window_flags flags)
{
    /* Decide which acceleration modes to try
     * and which are required to succeed */
    const bool try_vulkan = (flags & P_WINDOW_PREFER_ACCELERATED)
        || (flags & P_WINDOW_REQUIRE_ACCELERATED)
        || (flags & P_WINDOW_REQUIRE_VULKAN);
    const bool warn_vulkan = (flags & P_WINDOW_PREFER_ACCELERATED)
        || (flags & P_WINDOW_REQUIRE_ACCELERATED);
    const bool require_vulkan = (flags & P_WINDOW_REQUIRE_VULKAN) || 0;

    const bool try_opengl = (flags & P_WINDOW_PREFER_ACCELERATED)
        || (flags & P_WINDOW_REQUIRE_ACCELERATED)
        || (flags & P_WINDOW_REQUIRE_OPENGL);
    const bool warn_opengl = (flags & P_WINDOW_PREFER_ACCELERATED) || 0;
    const bool require_opengl = (flags & P_WINDOW_REQUIRE_OPENGL)
        || (flags & P_WINDOW_REQUIRE_ACCELERATED);

    const bool try_software = (flags & P_WINDOW_PREFER_ACCELERATED)
        || (flags & P_WINDOW_NO_ACCELERATION);

    win->info.gpu_acceleration = P_WINDOW_ACCELERATION_UNSET_;
    if (try_vulkan) {
        if (p_window_set_acceleration(win, P_WINDOW_ACCELERATION_VULKAN)) {
            if (require_vulkan) {
                s_log_error("Failed to initialize Vulkan.");
                return 1;
            } else if (warn_vulkan && try_opengl) {
                s_log_warn("Failed to initialize Vulkan. "
                    "Falling back to OpenGL.");
            } else if (warn_vulkan && try_software) {
                s_log_warn("Failed to initialize Vulkan. "
                    "Falling back to software rendering.");
            } else if (warn_vulkan) {
                s_log_warn("Failed to initialize Vulkan.");
            }
        } else {
            s_log_verbose("OK initializing Vulkan acceleration.");
            return 0;
        }
    }

    if (try_opengl) {
        if (p_window_set_acceleration(win, P_WINDOW_ACCELERATION_OPENGL)) {
            if (require_opengl) {
                s_log_error("Failed to initialize OpenGL.");
                return 1;
            } else if (warn_opengl && try_software) {
                s_log_warn("Failed to initialize OpenGL. "
                    "Falling back to software.");
            } else if (warn_opengl) {
                s_log_warn("Failed to initialize OpenGL.");
            }
        } else {
            s_log_verbose("OK initializing OpenGL acceleration.");
            return 0;
        }
    }

    if (try_software) {
        if (p_window_set_acceleration(win, P_WINDOW_ACCELERATION_NONE)) {
            s_log_error("Failed to initialize software rendering.");
            return 1;
        } else {
            s_log_verbose("OK initializing software rendering.");
            return 0;
        }
    }

    s_log_error("No GPU acceleration mode could be initialized.");
    return 1;
}

static i32 render_init_software(struct window_render_sw_ctx *sw_rctx,
    const struct p_window *win)
{
    memset(sw_rctx, 0, sizeof(struct window_render_sw_ctx));

    const u32 w = win->info.client_area.w;
    const u32 h = win->info.client_area.h;

    /* Initialize the pixel buffers */
    sw_rctx->buffers_[0].w = sw_rctx->buffers_[1].w = w;
    sw_rctx->buffers_[0].h = sw_rctx->buffers_[1].h = h;

    sw_rctx->buffers_[0].buf = calloc(w * h, sizeof(pixel_t));
    if (sw_rctx->buffers_[0].buf == NULL)
        goto_error("Failed to allocate the first pixel buffer");

    sw_rctx->buffers_[1].buf = calloc(w * h, sizeof(pixel_t));
    if (sw_rctx->buffers_[1].buf == NULL)
        goto_error("Failed to allocate the second pixel buffer");

    sw_rctx->back_fb = &sw_rctx->buffers_[0];
    sw_rctx->front_fb = &sw_rctx->buffers_[0];

    /* Get the device context of the window */
    sw_rctx->dc = GetDC(win->win);
    if (sw_rctx->dc == NULL)
        goto_error("Failed to get the window device context handle: %s",
            get_last_error_msg());

    return 0;

err:
    render_destroy_software(sw_rctx, win);
    return 1;
}

static struct pixel_flat_data * render_present_software(
    struct window_render_sw_ctx *sw_rctx,
    enum p_window_present_mode present_mode
)
{
    (void) present_mode;

    /* Swap the buffers */
    struct pixel_flat_data *const tmp = sw_rctx->back_fb;
    sw_rctx->back_fb = sw_rctx->front_fb;
    sw_rctx->front_fb = tmp;

    const BITMAPINFO bmi = {
        .bmiHeader = {
            .biSize = sizeof(BITMAPINFO),
            .biWidth = sw_rctx->front_fb->w,
            .biHeight = -sw_rctx->front_fb->h,
            .biPlanes = 1,
            .biBitCount = 32,
            .biCompression = BI_RGB
        },
    };
#define DST_X 0
#define DST_Y 0
#define SRC_X 0
#define SRC_Y 0
#define WIDTH sw_rctx->front_fb->w
#define HEIGHT sw_rctx->front_fb->h
#define START_SCANLINE 0
#define N_LINES HEIGHT

    i32 ret = SetDIBitsToDevice(sw_rctx->dc, DST_X, DST_Y, WIDTH, HEIGHT,
        SRC_X, SRC_Y, START_SCANLINE, N_LINES,
        sw_rctx->front_fb->buf, &bmi, DIB_RGB_COLORS);
    if (ret != (i32)HEIGHT) {
        s_log_error("SetDIBitsToDevice failed (ret %d - should be %d)",
            ret, HEIGHT);
        p_event_send(&(const struct p_event) {
            .type = P_EVENT_PAGE_FLIP,
            .info.page_flip_status = 1
        });
    } else {
        p_event_send(&(const struct p_event) {
            .type = P_EVENT_PAGE_FLIP,
            .info.page_flip_status = 0,
        });
    }

    return sw_rctx->back_fb;
}

static void render_destroy_software(struct window_render_sw_ctx *sw_rctx,
    const struct p_window *win)
{
    if (sw_rctx->dc != NULL) {
        /* Release the window's device context */
        if (ReleaseDC(win->win, sw_rctx->dc) != 1)
            s_log_error("The window device context was not released.");
    }

    if (sw_rctx->buffers_[0].buf != NULL)
        free(sw_rctx->buffers_[0].buf);
    if (sw_rctx->buffers_[1].buf != NULL)
        free(sw_rctx->buffers_[1].buf);

    memset(sw_rctx, 0, sizeof(struct window_render_sw_ctx));
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
