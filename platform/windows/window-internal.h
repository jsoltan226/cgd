#ifndef P_INTERNAL_GUARD__
#error This header file is internal to the cgd platform module and is not intended to be used elsewhere
#endif /* P_INTERNAL_GUARD__ */

#include "../thread.h"
#include "../window.h"
#include <core/pixel.h>
#include <core/shapes.h>
#include <stdbool.h>
#ifndef WIN32_LEAN_AND_MEAN
#define WINWIN32_LEAN_AND_MEAN
#endif /* WIN32_LEAN_AND_MEAN */
#include <windows.h>
#include <windef.h>
#include <wingdi.h>
#include <minwindef.h>

union window_render_ctx {
    struct window_render_sw_ctx {
        HDC dc; /* The device context of the window */

        /* The pixel buffers to which everything is rendered */
        struct pixel_flat_data buffers_[2];
        struct pixel_flat_data *back_fb, *front_fb;
    } sw;
};

struct p_window {
    HWND win; /* The handle to the main window */

    p_mt_thread_t thread; /* The window thread */
    bool initialized; /* Sanity check to avoid double-frees */

    RECT window_rect; /* The posistion and dimensions of the whole window */

    /* Contains the client area and the display dimensions */
    struct p_window_info info;

    /* Everything related to presenting the rendered contents to the window */
    union window_render_ctx render;
};

struct window_init {
    /* The p_window structure to be initialized */
    struct p_window *win;

    /* PARAMS */
    const char *title;
    const rect_t *area;
    const u32 flags;

    /* Temporary thread sync/communication objects */
    p_mt_cond_t cond;
    p_mt_mutex_t mutex;

    i32 result;
};
