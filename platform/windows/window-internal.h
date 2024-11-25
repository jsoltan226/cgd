#ifndef P_INTERNAL_GUARD__
#error This header file is internal to the cgd platform module and is not intended to be used elsewhere
#endif /* P_INTERNAL_GUARD__ */

#include "../thread.h"
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

struct p_window {
    HWND win; /* The handle to the main window */

    p_mt_thread_t thread; /* The window thread */
    bool initialized; /* Sanity check to avoid double-frees */

    RECT window_rect; /* The posistion and dimensions of the whole window */
    rect_t client_rect; /* The position and dimensions of the client area */

    /* Screen resolution, used for positioning the window */
    u32 screen_w, screen_h;

    HDC dc; /* The device context of the window */
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
