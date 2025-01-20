#define P_INTERNAL_GUARD__
#include "wm.h"
#undef P_INTERNAL_GUARD__
#define P_INTERNAL_GUARD__
#include "window-internal.h"
#undef P_INTERNAL_GUARD__
#define P_INTERNAL_GUARD__
#include "window-acceleration.h"
#undef P_INTERNAL_GUARD__
#include <core/int.h>
#include <core/log.h>
#include <core/util.h>
#include <stdlib.h>
#include <string.h>

#define MODULE_NAME "wm"

static i32 init_dri_software(struct wm *wm, struct p_window *win);
static i32 init_dri_egl(struct wm *wm, struct p_window *win);
static i32 init_dri_vulkan(struct wm *wm, struct p_window *win);
static i32 init_fbdev(struct wm *wm, struct p_window *win);

i32 wm_init(struct wm *wm, struct p_window *win)
{
    u_check_params(wm != NULL && win != NULL);
    memset(wm, 0, sizeof(struct wm));

    wm->win = win;
    wm->acceleration = win->gpu_acceleration;

    wm->client_area.x = win->x;
    wm->client_area.y = win->y;
    wm->client_area.w = win->w;
    wm->client_area.h = win->h;

    switch (win->type) {
    case WINDOW_TYPE_X11:
        goto_error("X11 windows cannot be managed by the WM!");
    case WINDOW_TYPE_DRI:
        switch (wm->acceleration) {
        case WINDOW_ACCELERATION_NONE:
            return init_dri_software(wm, win);
        case WINDOW_ACCELERATION_EGL_OPENGL:
            return init_dri_egl(wm, win);
        case WINDOW_ACCELERATION_VULKAN:
            return init_dri_vulkan(wm, win);
        default:
            goto_error("Unsupported DRI window acceleration mode");
        }
        break;
    case WINDOW_TYPE_FBDEV:
        return init_fbdev(wm, win);
    case WINDOW_TYPE_DUMMY:
        goto_error("Dummy windows cannot be managed by the WM!");
    }

err:
    memset(wm, 0, sizeof(struct wm));
    return 1;
}

void wm_destroy(struct wm *wm)
{
}

void wm_draw_decorations(struct wm *wm)
{
}

void wm_draw_cursor(struct wm *wm)
{
}

void wm_draw_borders(struct wm *wm)
{
}

static i32 init_dri_software(struct wm *wm, struct p_window *win)
{
    return 1;
}

static i32 init_dri_egl(struct wm *wm, struct p_window *win)
{
    return 1;
}

static i32 init_dri_vulkan(struct wm *wm, struct p_window *win)
{
    s_log_error("Vulkan acceleration is not yet implemented.");
    return 1;
}

static i32 init_fbdev(struct wm *wm, struct p_window *win)
{
    return 1;
}
