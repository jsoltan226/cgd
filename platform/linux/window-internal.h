#ifndef WINDOW_INTERNAL_H_
#define WINDOW_INTERNAL_H_

#ifndef P_INTERNAL_GUARD__
#error This header file is internal to the cgd platform module and is not intended to be used elsewhere
#endif /* P_INTERNAL_GUARD__ */

#include "../window.h"
#include <core/int.h>
#include <core/pixel.h>
#include <core/shapes.h>
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

struct window_generic_info {
    rect_t client_area;

    rect_t display_area;
    pixelfmt_t display_color_format;

    enum p_window_acceleration gpu_acceleration;
    bool vsync_supported;
};

struct p_window {
    struct p_window_info info;
    vec2d_t mouse_ev_offset;
    enum window_type type;
    union {
        struct window_x11 x11;
        struct window_dri dri;
        struct window_fbdev fbdev;
        struct window_dummy dummy;
    };
    struct wm wm; /* Only used by `fbdev` and `dri` */
};

#undef WINDOW_TYPE_LIST
#endif /* WINDOW_INTERNAL_H_ */
