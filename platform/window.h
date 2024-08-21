#ifndef P_WINDOW_H_
#define P_WINDOW_H_

#include "core/int.h"
#include "core/pixel.h"
#include "core/shapes.h"

struct p_window;

enum p_window_flags {
    WINDOW_NORMAL       = 1 << 0,
    WINDOW_FRAMEBUFFER  = 1 << 1,
};

struct p_window_meta {
    i32 x, y, w, h;
    enum p_window_color_type {
        P_WINDOW_RGBA8888,
        P_WINDOW_BGRA8888,
    } color_type;
};

struct p_window * p_window_open(i32 x, i32 y, i32 w, i32 h, u32 flags);
void p_window_close(struct p_window *win);

void p_window_render(struct p_window *win, pixel_t *frame, rect_t *area);
i32 p_window_get_meta(struct p_window *win, struct p_window_meta *out);

#endif /* P_WINDOW_H_ */
