#ifndef WINDOW_INTERNAL_H_
#define WINDOW_INTERNAL_H_

#ifndef P_INTERNAL_GUARD__
#error This header file is internal to the cgd platform module and is not intended to be used elsewhere
#endif /* P_INTERNAL_GUARD__ */

#include <core/int.h>
#include <core/pixel.h>
#include <core/shapes.h>
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
#define P_INTERNAL_GUARD__
#include "window-acceleration.h"
#undef P_INTERNAL_GUARD__

#define N_WINDOW_TYPES 4
#define WINDOW_TYPE_LIST    \
    X_(X11,     0)          \
    X_(DRI,     1)          \
    X_(FBDEV,   2)          \
    X_(DUMMY,   3)          \

#define X_(name, id) WINDOW_TYPE_##name = id,
enum window_type {
    WINDOW_TYPE_LIST
};
#undef X_
#define X_(name, id) WINDOW_BIT_##name = 1 << id,
enum window_bit {
    WINDOW_TYPE_LIST
};
#undef X_

#define X_(name, id) "WINDOW_TYPE_" #name,
static const char * const window_type_strings[N_WINDOW_TYPES] = {
    WINDOW_TYPE_LIST
};
#undef X_

struct p_window {
    enum window_type type;

    i32 x, y, w, h;

    union {
        struct window_x11 x11;
        struct window_dri dri;
        struct window_fbdev fbdev;
        struct window_dummy dummy;
    };

    pixelfmt_t color_format;
    vec2d_t ev_offset;
    enum window_acceleration gpu_acceleration;
};

static inline enum window_acceleration
window_get_acceleration(struct p_window *win)
{
    return win->gpu_acceleration;
}

/* Implemented in `platform/linux/window.c` */
void window_set_acceleration(struct p_window *win,
    enum window_acceleration val);

#undef WINDOW_TYPE_LIST
#endif /* WINDOW_INTERNAL_H_ */
