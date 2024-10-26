#include "../event.h"
#include "../window.h"
#include <core/int.h>
#include <core/log.h>
#include <core/util.h>
#include <core/pixel.h>
#include <core/shapes.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#define P_INTERNAL_GUARD__
#include "window-fbdev.h"
#undef P_INTERNAL_GUARD__
#define P_INTERNAL_GUARD__
#include "window-x11.h"
#undef P_INTERNAL_GUARD__
#define P_INTERNAL_GUARD__
#include "window-dummy.h"
#undef P_INTERNAL_GUARD__
#define P_INTERNAL_GUARD__
#include "window-internal.h"
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
    win->bound_fb = NULL;

    if (flags & P_WINDOW_TYPE_NORMAL)
        win->type = WINDOW_TYPE_X11;
    else if (flags & P_WINDOW_TYPE_FRAMEBUFFER)
        win->type = WINDOW_TYPE_FRAMEBUFFER;
    else if (flags & P_WINDOW_TYPE_DUMMY) {
        win->type = WINDOW_TYPE_DUMMY;
    } else /* if (flags & P_WINDOW_TYPE_AUTO) */
        win->type = detect_environment();

    /* Init the event subsystem so that the user doesn't have to */
    p_event_send(&(struct p_event) { .type = P_EVENT_CTL_INIT_ });

    switch (win->type) {
        case WINDOW_TYPE_X11:
            if (window_X11_open(&win->x11, title, area, flags))
                goto_error("Failed to open X11 window");
            win->color_type = P_WINDOW_BGRA8888;
            win->ev_offset.x = 0;
            win->ev_offset.y = 0;
            break;
        case WINDOW_TYPE_FRAMEBUFFER:
            if (window_fbdev_open(&win->fbdev, area, flags))
                goto_error("Failed to open framebuffer window");
            win->color_type = P_WINDOW_BGRA8888;
            win->ev_offset.x = win->x;
            win->ev_offset.y = win->y;
            break;
        case WINDOW_TYPE_DUMMY:
            window_dummy_init(&win->dummy);
            win->color_type = P_WINDOW_RGBA8888;
            break;
    }
    
    s_log_info("%s() OK, window type is \"%s\"",
        __func__, window_type_strings[win->type]);
    return win;
err:
    p_window_close(&win);
    return NULL;
}

void p_window_bind_fb(struct p_window *win, struct pixel_flat_data *fb)
{
    u_check_params(win != NULL && fb != NULL);
    s_assert(fb->w == win->w && fb->h == win->h,
        "%s: Invalid framebuffer size", __func__);
    s_assert(win->bound_fb == NULL,
        "A framebuffer is already bound to this window.");

    win->bound_fb = fb;
    if (win->type == WINDOW_TYPE_X11)
        window_X11_bind_fb(&win->x11, fb);
}

void p_window_unbind_fb(struct p_window *win)
{
    u_check_params(win != NULL);
    
    if (win->type == WINDOW_TYPE_X11)
        window_X11_unbind_fb(&win->x11);

    win->bound_fb = NULL;
}

void p_window_close(struct p_window **win_p)
{
    if (win_p == NULL || *win_p == NULL) return;

    struct p_window *win = *win_p;

    p_window_unbind_fb(win);
    switch (win->type) {
        case WINDOW_TYPE_X11:
            window_X11_close(&win->x11);
            break;
        case WINDOW_TYPE_FRAMEBUFFER:
            window_fbdev_close(&win->fbdev);
            break;
        case WINDOW_TYPE_DUMMY:
            window_dummy_destroy(&win->dummy);
            break;
    }

    u_nzfree(win_p);
    p_event_send(&(struct p_event) { .type = P_EVENT_CTL_DESTROY_ });
}

void p_window_render(struct p_window *win)
{
    u_check_params(win != NULL);

    switch (win->type) {
        case WINDOW_TYPE_X11:
            window_X11_render(&win->x11, win->bound_fb);
            break;
        case WINDOW_TYPE_FRAMEBUFFER:
            window_fbdev_render_to_display(&win->fbdev, win->bound_fb);
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
