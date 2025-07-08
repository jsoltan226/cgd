#ifndef WINDOW_INTERNAL_H_
#define WINDOW_INTERNAL_H_

#ifndef P_INTERNAL_GUARD__
#error This header file is internal to the cgd platform module and is not intended to be used elsewhere
#endif /* P_INTERNAL_GUARD__ */

#define P_INTERNAL_GUARD__
#include "window-present-sw.h"
#undef P_INTERNAL_GUARD__
#include "../thread.h"
#include "../window.h"
#include <core/pixel.h>
#include <core/shapes.h>
#include <stdbool.h>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif /* WIN32_LEAN_AND_MEAN */
#include <windows.h>
#include <windef.h>
#include <wingdi.h>
#include <minwindef.h>


#define CGD_REQEV_QUIT_ (WM_APP + 1)
#define CGD_REQEV_RENDER_PRESENT_SOFTWARE (WM_APP + 2)

union window_render_ctx {
    struct window_render_software_ctx sw;
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

    /* The window's GDI device context */
    HDC window_dc;
};

struct window_init {
    /* The p_window structure to be initialized */
    struct p_window *win;

    /* PARAMS */
    struct {
        const char *title;
        const rect_t *area;
        const u32 flags;
    } in;

    /* Data returned by the thread */
    struct {
        HDC window_dc;
    } out;

    /* Temporary thread sync/communication objects */
    p_mt_cond_t cond;
    p_mt_mutex_t mutex;

    _Atomic i32 result;
};

#endif /* WINDOW_INTERNAL_H_ */
