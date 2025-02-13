#define P_INTERNAL_GUARD__
#include "wm.h"
#undef P_INTERNAL_GUARD__
#define P_INTERNAL_GUARD__
#include "window-internal.h"
#undef P_INTERNAL_GUARD__
#include <core/int.h>
#include <core/log.h>
#include <core/util.h>
#include <core/pixel.h>

#define MODULE_NAME "wm"

i32 wm_init(struct wm *wm, struct p_window *win)
{
    return 1;
}

void wm_destroy(struct wm *wm)
{
}

void wm_draw_client(struct wm *wm, const struct pixel_flat_data *frame)
{
    switch (wm->win->type) {
    case WINDOW_TYPE_FBDEV:
    case WINDOW_TYPE_DRI:
    default:
        return;
    }
}
