#include "../window.h"
#include <stdlib.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <string.h>
#include "core/int.h"
#include "core/log.h"
#include "core/math.h"
#include "core/util.h"
#define P_INTERNAL_GUARD__
#include "window-fb.h"
#undef P_INTERNAL_GUARD__
#define P_INTERNAL_GUARD__
#include "window-x11.h"
#undef P_INTERNAL_GUARD__
#define P_INTERNAL_GUARD__
#include "window-internal.h"
#undef P_INTERNAL_GUARD__

#define MODULE_NAME "window"

struct p_window * p_window_open(i32 x, i32 y, i32 w, i32 h, u32 flags)
{
    struct p_window *win = calloc(1, sizeof(struct p_window));
    s_assert(win != NULL, "calloc() failed for struct window!");

    win->x = x;
    win->y = y;
    win->w = w;
    win->h = h;

    if (flags & WINDOW_NORMAL)
        win->type = WINDOW_TYPE_X11;
    else if (flags & WINDOW_FRAMEBUFFER)
        win->type = WINDOW_TYPE_FRAMEBUFFER;
    else
        win->type = WINDOW_TYPE_X11;

    switch (win->type) {
        case WINDOW_TYPE_X11:
            if (window_X11_open(&win->x11, x, y, w, h))
                goto_error("Failed to open X11 window");
            win->color_type = P_WINDOW_RGBA8888;
            break;
        case WINDOW_TYPE_FRAMEBUFFER:
            if (window_fb_open(&win->fb, &(rect_t){ x, y, w, h }))
                goto_error("Failed to open framebuffer window");
            win->color_type = P_WINDOW_BGRA8888;
            break;
    }
    
    return win;
err:
    p_window_close(win);
    return NULL;
}

void p_window_close(struct p_window *win)
{
    if (win == NULL) return;

    switch (win->type) {
        case WINDOW_TYPE_X11:
            window_X11_close(&win->x11);
            break;
        case WINDOW_TYPE_FRAMEBUFFER:
            window_fb_close(&win->fb);
            break;
    }

    free(win);
}

void p_window_render(struct p_window *win, pixel_t *buf, rect_t *frame)
{
    if (win == NULL || buf == NULL || frame == NULL)
        return;

    rect_t clipped_frame;
    memcpy(&clipped_frame, frame, sizeof(rect_t));

    switch (win->type) {
        case WINDOW_TYPE_X11:
            break;
        case WINDOW_TYPE_FRAMEBUFFER:
            clipped_frame.x += win->x;
            clipped_frame.y += win->y;
            clipped_frame.w = min(win->w, clipped_frame.w);
            clipped_frame.h = min(win->h, clipped_frame.h);
            window_fb_render_to_display(&win->fb, buf, &clipped_frame);
            break;
    }
}

i32 p_window_get_meta(struct p_window *win, struct p_window_meta *out)
{
    if (win == NULL || out == NULL) {
        s_log_error("%s: invalid parameters", __func__);
        return 1;
    }

    out->x = win->x;
    out->y = win->y;
    out->w = win->w;
    out->h = win->h;

    out->color_type = win->color_type;

    return 0;
}
