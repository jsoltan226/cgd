#include "../window.h"
#include <stdlib.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <string.h>
#include "core/int.h"
#include "core/log.h"
#include "core/pixel.h"
#include "core/shapes.h"
#include "core/util.h"
#include "../event.h"
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
            dummy_window_init(&win->dummy);
            win->color_type = P_WINDOW_RGBA8888;
            break;
    }
    
    /* Init the event subsystem so that the user doesn't have to */
    p_event_send(&(struct p_event) { .type = P_EVENT_CTL_INIT_ });
    return win;
err:
    p_window_close(win);
    return NULL;
}

void p_window_bind_fb(struct p_window *win, struct pixel_flat_data *fb)
{
    u_check_params(win != NULL && fb != NULL);
    s_assert(fb->w == win->w && fb->h == win->h,
        "%s: Invalid framebuffer size", __func__);

    switch(win->type) {
        case WINDOW_TYPE_X11:
            window_X11_bind_fb(&win->x11, fb);
            break;
        case WINDOW_TYPE_FRAMEBUFFER:
            window_fb_bind_fb(&win->fb, fb);
            break;
        default: case WINDOW_TYPE_DUMMY:
            s_log_fatal(MODULE_NAME, __func__,
                "Attempt to bind framebuffer to a window of invalid type");
            break;
    }
}

void p_window_unbind_fb(struct p_window *win, bool free_buf)
{
    u_check_params(win != NULL);
    switch(win->type) {
        case WINDOW_TYPE_X11:
            window_X11_unbind_fb(&win->x11, free_buf);
            break;
        case WINDOW_TYPE_FRAMEBUFFER:
            window_fb_unbind_fb(&win->fb, free_buf);
            break;
        default: case WINDOW_TYPE_DUMMY:
            break;
    }
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
    p_event_send(&(struct p_event) { .type = P_EVENT_CTL_DESTROY_ });
}

void p_window_render(struct p_window *win)
{
    u_check_params(win != NULL);

    switch (win->type) {
        case WINDOW_TYPE_X11:
            window_X11_render(&win->x11);
            break;
        case WINDOW_TYPE_FRAMEBUFFER:
            window_fb_render_to_display(&win->fb);
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
