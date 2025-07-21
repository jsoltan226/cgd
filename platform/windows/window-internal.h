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

union window_render_ctx {
    struct window_render_software_ctx sw;
};

struct p_window {
    _Atomic bool exists_; /* Sanity check to avoid double-frees */

    _Atomic bool thread_started_;
    p_mt_thread_t thread; /* The window thread */

    RECT window_rect; /* The posistion and dimensions of the whole window */

    /* Contains the client area and the display dimensions */
    struct p_window_info info;

    /* Everything related to presenting the rendered contents to the window */
    union window_render_ctx render;

    /* The handle to the window created & owned by the window thread */
    HWND win_handle;

    _Atomic bool init_ok_; /* Used to mark successfull window creation */
};

struct window_init {
    /* PARAMS */
    struct {
        const char *title;
        u32 flags;
        const struct p_window_info *win_info;
    } in;

    /* Data returned by the thread */
    struct {
        HWND win_handle;
        RECT window_rect;
    } out;

    /* Temporary thread sync/communication objects */
    p_mt_cond_t cond;
    p_mt_mutex_t mutex;

    i32 result;
};

#endif /* WINDOW_INTERNAL_H_ */
