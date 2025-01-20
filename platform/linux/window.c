#include "../event.h"
#include "../window.h"
#include <core/int.h>
#include <core/log.h>
#include <core/util.h>
#include <core/pixel.h>
#include <core/shapes.h>
#include <stdlib.h>
#include <string.h>
#define P_INTERNAL_GUARD__
#include "window-internal.h"
#undef P_INTERNAL_GUARD__
#define P_INTERNAL_GUARD__
#include "window-x11.h"
#undef P_INTERNAL_GUARD__
#define P_INTERNAL_GUARD__
#include "window-dri.h"
#undef P_INTERNAL_GUARD__
#define P_INTERNAL_GUARD__
#include "window-fbdev.h"
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

    if (flags & P_WINDOW_TYPE_DUMMY && flags & P_WINDOW_REQUIRE_ACCELERATED) {
        s_log_error("Contradicting flags: %s and %s",
            "P_WINDOW_TYPE_DUMMY", "P_WINDOW_REQUIRED_ACCELERATED");
        return NULL;
    }

    struct p_window *win = calloc(1, sizeof(struct p_window));
    s_assert(win != NULL, "calloc() failed for struct window!");

    win->x = area->x;
    win->y = area->y;
    win->w = area->w;
    win->h = area->h;

    if (flags & P_WINDOW_TYPE_DUMMY)
        win->type = WINDOW_TYPE_DUMMY;
    else
        win->type = detect_environment();

    /* Init the event subsystem so that the user doesn't have to */
    p_event_send(&(const struct p_event) { .type = P_EVENT_CTL_INIT_ });

    switch (win->type) {
        case WINDOW_TYPE_X11:
            if (window_X11_open(&win->x11, title, area, flags))
                goto_error("Failed to open X11 window");
            win->color_format = BGRX32;
            win->ev_offset.x = 0;
            win->ev_offset.y = 0;
            break;
        case WINDOW_TYPE_DRI:
            if (window_dri_open(&win->dri, area, flags)) {
                s_log_warn("Failed to open DRI window. Falling back to fbdev.");
                win->type = WINDOW_TYPE_FBDEV;
                goto fbdev_init;
            }
            win->color_format = BGRX32;
            win->ev_offset.x = win->x;
            win->ev_offset.y = win->y;
            break;
        case WINDOW_TYPE_FBDEV:
        fbdev_init:
            if (window_fbdev_open(&win->fbdev, area, flags))
                goto_error("Failed to open fbdev window");
            win->color_format = BGRX32;
            win->ev_offset.x = win->x;
            win->ev_offset.y = win->y;
            break;
        case WINDOW_TYPE_DUMMY:
            if (window_dummy_init(&win->dummy, flags))
                goto_error("Failed to init dummy window");
            win->color_format = RGBX32;
            win->ev_offset.x = 0;
            win->ev_offset.y = 0;
            break;
    }

    s_log_info("%s() OK, window type is \"%s\"",
        __func__, window_type_strings[win->type]);
    return win;
err:
    p_window_close(&win);
    return NULL;
}

void p_window_close(struct p_window **win_p)
{
    if (win_p == NULL || *win_p == NULL) return;

    struct p_window *win = *win_p;

    switch (win->type) {
        case WINDOW_TYPE_X11:
            window_X11_close(&win->x11);
            break;
        case WINDOW_TYPE_DRI:
            window_dri_close(&win->dri);
            break;
        case WINDOW_TYPE_FBDEV:
            window_fbdev_close(&win->fbdev);
            break;
        case WINDOW_TYPE_DUMMY:
            window_dummy_destroy(&win->dummy);
            break;
    }

    u_nzfree(win_p);
    p_event_send(&(const struct p_event) { .type = P_EVENT_CTL_DESTROY_ });
}

void p_window_render(struct p_window *win, struct pixel_flat_data *fb)
{
    u_check_params(win != NULL);

    if (fb != NULL) {
        s_assert(win->w == fb->w && win->h == fb->h,
            "Provided buffer has invalid dimensions "
            "(window: %ux%u, buf: %ux%u)",
            win->w, win->h, fb->w, fb->h);
    }

    switch (win->type) {
        case WINDOW_TYPE_X11:
            window_X11_render(&win->x11, fb);
            break;
        case WINDOW_TYPE_DRI:
            window_dri_render(&win->dri, fb);
            break;
        case WINDOW_TYPE_FBDEV:
            window_fbdev_render_to_display(&win->fbdev, fb);
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

    out->color_format = win->color_format;

    return 0;
}

/* From `window-internal.h` */
void window_set_acceleration(struct p_window *win,
    enum window_acceleration val)
{
    u_check_params(win != NULL);

    switch (val) {
    case WINDOW_ACCELERATION_NONE:
        break;
    case WINDOW_ACCELERATION_EGL_OPENGL:
        if (win->gpu_acceleration == WINDOW_ACCELERATION_EGL_OPENGL) {
            s_log_warn("Attempt to enable EGL/OpenGL acceleration twice. "
                "Not doing anything.");
            break;
        } else if (win->gpu_acceleration != WINDOW_ACCELERATION_NONE) {
            s_log_fatal(MODULE_NAME, __func__,
                "Attempt to enable gpu acceleration while another "
                "acceleration mode is still active. Stop.");
        }
        break;
    case WINDOW_ACCELERATION_VULKAN:
        s_log_fatal(MODULE_NAME, __func__,
            "Vulkan acceleration not implemented yet");
    default:
        s_log_fatal(MODULE_NAME, __func__,
            "Invalid parameters (`val`: %)", val);
    }

    switch (win->type) {
    case WINDOW_TYPE_X11:
        window_X11_set_acceleration(&win->x11, val);
        break;
    case WINDOW_TYPE_DRI:
        window_dri_set_acceleration(&win->dri, val);
        break;
    case WINDOW_TYPE_FBDEV:
    case WINDOW_TYPE_DUMMY:
        s_assert(val == WINDOW_ACCELERATION_NONE,
            "Dummy and fbdev windows don't support GPU acceleration!");
        break;
    }
}

static enum window_type detect_environment()
{
    /* Try X11 first. If X is active,
     * writing to framebuffer device is at best useless */
    if (getenv("DISPLAY"))
        return WINDOW_TYPE_X11;

    /* If nothing is detected, DRI/fbdev are the only choices */
    return WINDOW_TYPE_DRI;
}
