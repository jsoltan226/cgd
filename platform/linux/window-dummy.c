#include "core/pixel.h"
#include "core/util.h"
#include <stdlib.h>
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
    window_dummy_unbind_fb(win);
    memset(win, 0, sizeof(struct window_dummy));
}

void window_dummy_bind_fb(struct window_dummy *win, struct pixel_flat_data *fb)
{
    win->bound_fb = fb;
}

void window_dummy_unbind_fb(struct window_dummy *win)
{
    if (win->bound_fb == NULL || win->bound_fb->buf == NULL)
        return;

    free(win->bound_fb->buf);
    memset(win->bound_fb, 0, sizeof(struct pixel_flat_data));
    win->bound_fb = NULL;
}
