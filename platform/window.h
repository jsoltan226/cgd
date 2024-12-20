#ifndef P_WINDOW_H_
#define P_WINDOW_H_

#include <core/int.h>
#include <core/pixel.h>
#include <core/shapes.h>
#include <stdbool.h>

struct p_window;

enum p_window_flags {
    P_WINDOW_TYPE_NORMAL        = 1 << 0,
    P_WINDOW_TYPE_DUMMY         = 1 << 3,

    P_WINDOW_POS_CENTERED_X     = 1 << 10,
    P_WINDOW_POS_CENTERED_Y     = 1 << 11,
};
#define P_WINDOW_POS_CENTERED_XY \
    (P_WINDOW_POS_CENTERED_X | P_WINDOW_POS_CENTERED_Y)

struct p_window_meta {
    i32 x, y, w, h;
    pixelfmt_t color_format;
};

struct p_window * p_window_open(const unsigned char *title,
    const rect_t *area, const u32 flags);

void p_window_render(struct p_window *win, struct pixel_flat_data *fb);

void p_window_close(struct p_window **win_p);

i32 p_window_get_meta(const struct p_window *win, struct p_window_meta *out);

#endif /* P_WINDOW_H_ */
