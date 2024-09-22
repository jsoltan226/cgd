#include "../window.h"
#include "core/util.h"
#include "core/int.h"
#include <pthread.h>
#define P_INTERNAL_GUARD__
#include "window-x11.h"
#undef P_INTERNAL_GUARD__
#define P_INTERNAL_GUARD__
#include "window-dummy.h"
#undef P_INTERNAL_GUARD__

#define MODULE_NAME "window-dummy"

i32 dummy_window_init(struct window_dummy *win, bool is_X11)
{
    memset(win, 0, sizeof(struct window_dummy));

    win->is_X11 = is_X11;
    if (is_X11) {
        const rect_t window_rect = { 0, 0, 24, 24 };
        const u32 flags = P_WINDOW_TYPE_DUMMY;
        if (window_X11_open(
                &win->x11,
                (const unsigned char *)"dummy",
                &window_rect,
                flags
            )
        ) {
            goto_error("Dummy X11 window setup failed");
        }
    }

    return 0;

err:
    dummy_window_destroy(win);
    return 1;
}

void dummy_window_destroy(struct window_dummy *win)
{
    if (win->is_X11) window_X11_close(&win->x11);
    /* libX11 will be unloaded by window_X11_close() */
}
