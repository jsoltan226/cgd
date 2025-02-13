#define P_INTERNAL_GUARD__
#include "../window.h"
#undef P_INTERNAL_GUARD__
#include "../event.h"
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
#include "wm.h"
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

static i32 check_flags(const u32 flags);
static enum window_type detect_environment(void);

struct p_window * p_window_open(const unsigned char *title,
    const rect_t *area, const u32 flags)
{
    u_check_params(area != NULL);

    if (check_flags(flags))
        return NULL;

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
                window_dri_close(&win->dri);
                win->type = WINDOW_TYPE_FBDEV;
                goto fbdev_init;
            }
            if (wm_init(&win->wm, win)) {
                s_log_warn("Failed to initialize the window manager "
                    "for a DRI window. Falling back to fbdev.");
                wm_destroy(&win->wm);
                window_dri_close(&win->dri);
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
            if (wm_init(&win->wm, win)) {
                s_log_warn("Failed to initialize the window manager.");
                wm_destroy(&win->wm);
            }
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

void p_window_get_meta(const struct p_window *win, struct p_window_meta *out)
{
    u_check_params(win != NULL && out != NULL);

    out->x = win->x;
    out->y = win->y;
    out->w = win->w;
    out->h = win->h;

    out->color_format = win->color_format;
    out->acceleration = win->gpu_acceleration;
}

struct pixel_flat_data * p_window_swap_buffers(struct p_window *win,
    const enum p_window_present_mode present_mode)
{
    if (win->gpu_acceleration != P_WINDOW_ACCELERATION_NONE)
        return NULL;

    switch (win->type) {
        case WINDOW_TYPE_X11: return NULL;
        case WINDOW_TYPE_DRI: return NULL;
        case WINDOW_TYPE_FBDEV:
            return window_fbdev_swap_buffers(&win->fbdev, present_mode);
        case WINDOW_TYPE_DUMMY: return NULL;
    }

    return NULL;
}

i32 p_window_set_acceleration(struct p_window *win,
    enum p_window_acceleration new_acceleration_mode)
{
    u_check_params(win != NULL);

    switch (new_acceleration_mode) {
    case P_WINDOW_ACCELERATION_NONE:
        if (win->gpu_acceleration == P_WINDOW_ACCELERATION_NONE) {
            s_log_warn("Attempt to disable GPU acceleration "
                "when it's already disabled. Not doing anything.");
            return 0;
        }
        break;
    case P_WINDOW_ACCELERATION_OPENGL:
        if (win->gpu_acceleration == P_WINDOW_ACCELERATION_OPENGL) {
            s_log_warn("Attempt to enable EGL/OpenGL acceleration twice. "
                "Not doing anything.");
            return 0;
        } else if (win->gpu_acceleration != P_WINDOW_ACCELERATION_NONE) {
            s_log_error("Attempt to enable gpu acceleration while another "
                "acceleration mode is still active.");
            return 1;
        }
        break;
    case P_WINDOW_ACCELERATION_VULKAN:
        s_log_error("Vulkan acceleration not implemented yet");
        return 1;
    default:
        s_log_fatal(MODULE_NAME, __func__,
            "Invalid parameters (`new_acceleration_mode`: %u)",
            new_acceleration_mode);
    }

    switch (win->type) {
    case WINDOW_TYPE_X11:
        return window_X11_set_acceleration(&win->x11, new_acceleration_mode);
    case WINDOW_TYPE_DRI:
        return window_dri_set_acceleration(&win->dri, new_acceleration_mode);
    case WINDOW_TYPE_FBDEV:
    case WINDOW_TYPE_DUMMY:
        s_assert(new_acceleration_mode == P_WINDOW_ACCELERATION_NONE,
            "Dummy and fbdev windows don't support GPU acceleration!");
        return 0;
    }

    return 1;
}

static i32 check_flags(const u32 flags)
{
    static const enum p_window_flags contradicting_pairs[][2] = {
        { P_WINDOW_TYPE_NORMAL,  P_WINDOW_TYPE_DUMMY },
        { P_WINDOW_TYPE_DUMMY, P_WINDOW_REQUIRE_ACCELERATED },
        { P_WINDOW_TYPE_DUMMY, P_WINDOW_REQUIRE_VULKAN },
        { P_WINDOW_TYPE_DUMMY, P_WINDOW_REQUIRE_OPENGL },
        { P_WINDOW_PREFER_ACCELERATED, P_WINDOW_REQUIRE_ACCELERATED },
        { P_WINDOW_PREFER_ACCELERATED, P_WINDOW_REQUIRE_OPENGL },
        { P_WINDOW_PREFER_ACCELERATED, P_WINDOW_REQUIRE_VULKAN },
        { P_WINDOW_REQUIRE_ACCELERATED, P_WINDOW_REQUIRE_VULKAN },
        { P_WINDOW_REQUIRE_ACCELERATED, P_WINDOW_REQUIRE_OPENGL },
        { P_WINDOW_REQUIRE_VULKAN, P_WINDOW_REQUIRE_OPENGL }
    };
    i32 ret = 0;
    for (u32 i = 0; i < u_arr_size(contradicting_pairs); i++) {
        const u32 flag_a = contradicting_pairs[i][0];
        const u32 flag_b = contradicting_pairs[i][1];
        if (flags & flag_a && flags & flag_b) {
            i32 index_a = 0;
            i32 index_b = 0;
            while (flag_a != 1 << index_a) index_a++;
            while (flag_b != 1 << index_b) index_b++;

            s_log_error("Contradicting flags: \"%s\" and \"%s\"",
                p_window_flag_strings[index_a], p_window_flag_strings[index_b]);
            ret++;
        }
    }

    return ret;
}

static enum window_type detect_environment(void)
{
    /* Try X11 first. If X is active,
     * writing to framebuffer device is at best useless */
    if (getenv("DISPLAY"))
        return WINDOW_TYPE_X11;

    /* If nothing is detected, DRI/fbdev are the only choices */
    return WINDOW_TYPE_FBDEV;
}
