#include <core/pixel.h>
#include <string.h>
#define P_INTERNAL_GUARD__
#include "window-dummy.h"
#undef P_INTERNAL_GUARD__

#define MODULE_NAME "window-dummy"

void window_dummy_init(struct window_dummy *win)
{
    memset(win, 0, sizeof(struct window_dummy));
}

void window_dummy_destroy(struct window_dummy *win)
{
    memset(win, 0, sizeof(struct window_dummy));
}

void window_dummy_bind_fb(struct window_dummy *win, struct pixel_flat_data *fb)
{
}

void window_dummy_unbind_fb(struct window_dummy *win)
{
}
