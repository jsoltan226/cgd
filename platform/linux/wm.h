#ifndef WINDOW_MANAGER_H_
#define WINDOW_MANAGER_H_

#ifndef P_INTERNAL_GUARD__
#error This header file is internal to the cgd platform module and is not intended to be used elsewhere
#endif /* P_INTERNAL_GUARD__ */

#include <core/int.h>
#include <core/shapes.h>
#define P_INTERNAL_GUARD__
#include "window-internal.h"
#undef P_INTERNAL_GUARD__
#define P_INTERNAL_GUARD__
#include "window-acceleration.h"
#undef P_INTERNAL_GUARD__

struct wm_decorations {

};

struct wm_cursor {
    vec2d_t pos;
};

struct wm {
    rect_t client_area;
    struct wm_decorations decorations;
    struct wm_cursor cursor;

    struct p_window *win;

    enum window_acceleration acceleration;
    union wm_render_ctx {
        struct wm_software_render_ctx {
        } sw;
        struct wm_egl_render_ctx {
        } egl;
        struct wm_vulkan_render_ctx {
        } vk;
    } render;
};

i32 wm_init(struct wm *wm, struct p_window *win);
void wm_destroy(struct wm *wm);

void wm_draw_decorations(struct wm *wm);
void wm_draw_cursor(struct wm *wm);
void wm_draw_borders(struct wm *wm);

#define wm_draw_all(wm) do {    \
    wm_draw_decorations(wm);    \
    wm_draw_cursor(wm);         \
    wm_draw_borders(wm);        \
} while (0)

#endif /* WINDOW_MANAGER_H_ */
