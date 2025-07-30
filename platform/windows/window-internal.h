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

/* The internal definition of the actual `p_window` struct
 * for the windows implementation.
 * Note that the majority of the real functionality is located
 * elsewhere, particularly in the "window-thread"
 * and "window-present-sw" modules. */
struct p_window {
    _Atomic bool exists_; /* Sanity check to avoid double-frees */

    _Atomic bool thread_started_;
    p_mt_thread_t thread; /* The window thread */
    /* A pointer to the variable that tells the window thread
     * when to exit */
    _Atomic bool *thread_running_p;

    RECT window_rect; /* The position and dimensions of the whole window */

    /* Contains the client area and the display dimensions */
    struct p_window_info info;

    /* Everything related to presenting the rendered contents to the window */
    union window_render_ctx {
        struct window_render_software_ctx sw; /* Software rendering context */
    } render;

    /* The handle to the window created & owned by the window thread */
    HWND win_handle;

    _Atomic bool init_ok_; /* Used to mark successfull window creation */
};

/* The struct used to communicate with the window thread
 * during initialization */
struct window_init {
    /* Data given to the thread */
    struct {
        const char *title; /* The Desired window title */
        u32 flags; /* Window flags (unused for now) */

        /* An initialized `win_info` struct (so that the thread knows
         * the position and dimensions of the window to be created) */
        const struct p_window_info *win_info;
    } in;

    /* Data returned by the thread */
    struct {
        /* The handle to the new window.
         * Note that a lot of operations on it require being executed
         * from the thread that owns it, hence a messaging system is used
         * to request such operations from the window thread.
         * For more details, see `platform/windows/window-thread.h`. */
        HWND win_handle;

        /* The adjusted and converted-to-the-win32-stupid-format rect
         * containing the (initial) position and dimensions of the window */
        RECT window_rect;

        /* A pointer to the variable that tells the window thread
         * when to exit */
        _Atomic bool *running_p;
    } out;

    /* Temporary thread sync/communication objects */
    p_mt_cond_t cond;
    p_mt_mutex_t mutex;

    /* Here the thread stores the result of the initialization.
     * How unxpected, right? */
    i32 result;
};

#endif /* WINDOW_INTERNAL_H_ */
