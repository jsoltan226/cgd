#include "../window.h"
#include <stdlib.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <string.h>
#include "core/int.h"
#include "core/log.h"
#include "core/shapes.h"
#include "core/util.h"
#define P_INTERNAL_GUARD__
#include "window-internal.h"
#undef P_INTERNAL_GUARD__
#define P_INTERNAL_GUARD__
#include "window-x11.h"
#undef P_INTERNAL_GUARD__
#define P_INTERNAL_GUARD__
#include "window-fb.h"
#undef P_INTERNAL_GUARD__
#define P_INTERNAL_GUARD__
#include "window-dummy.h"
#undef P_INTERNAL_GUARD__

#define MODULE_NAME "window"

static enum window_type detect_environment();

struct p_window * p_window_open(const unsigned char *title,
    const rect_t *area, const u32 flags)
{
    u_check_params(area != NULL);

    struct p_window *win = calloc(1, sizeof(struct p_window));
    s_assert(win != NULL, "calloc() failed for struct window!");

    win->x = area->x;
    win->y = area->y;
    win->w = area->w;
    win->h = area->h;

    if (flags & P_WINDOW_TYPE_NORMAL)
        win->type = WINDOW_TYPE_X11;
    else if (flags & P_WINDOW_TYPE_FRAMEBUFFER)
        win->type = WINDOW_TYPE_FRAMEBUFFER;
    else if (flags & P_WINDOW_TYPE_DUMMY) {
        win->type = WINDOW_TYPE_DUMMY;
    } else /* if (flags & P_WINDOW_TYPE_AUTO) */
        win->type = detect_environment();

    switch (win->type) {
        case WINDOW_TYPE_X11:
            if (window_X11_open(&win->x11, title, area, flags))
                goto_error("Failed to open X11 window");
            win->color_type = P_WINDOW_BGRA8888;
            break;
        case WINDOW_TYPE_FRAMEBUFFER:
            if (window_fb_open(&win->fb, area, flags))
                goto_error("Failed to open framebuffer window");
            win->color_type = P_WINDOW_BGRA8888;
            break;
        case WINDOW_TYPE_DUMMY:
            if (dummy_window_init(&win->dummy,
                    detect_environment() == WINDOW_TYPE_X11)
            ) {
                goto_error("Failed to init dummy window");
            }
            win->color_type = P_WINDOW_RGBA8888;
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
        case WINDOW_TYPE_DUMMY:
            dummy_window_destroy(&win->dummy);
            break;
    }

    free(win);
}

void p_window_render(struct p_window *win,
    const pixel_t *data, const rect_t *frame)
{
    if (win == NULL || data == NULL || frame == NULL)
        return;

    switch (win->type) {
        case WINDOW_TYPE_X11:
            window_X11_render(&win->x11, data, frame);
            break;
        case WINDOW_TYPE_FRAMEBUFFER:
            window_fb_render_to_display(&win->fb, data, frame);
            break;
        case WINDOW_TYPE_DUMMY:
            /* Do nothing, it's a dummy lol */
            break;
    }
}

i32 p_window_get_meta(const struct p_window *win, struct p_window_meta *out)
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

static enum window_type detect_environment()
{
    /* Try X11 first. If X is active,
     * writing to framebuffer device is at best useless */
    if (getenv("DISPLAY"))
        return WINDOW_TYPE_X11;

    /* If nothing is detected, framebuffer dev is the only choice */
    return WINDOW_TYPE_FRAMEBUFFER;
}
