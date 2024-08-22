#ifndef WINDOW_INTERNAL_H_
#define WINDOW_INTERNAL_H_
#ifndef P_INTERNAL_GUARD__
#error This header file is internal to the cgd platform module and is not intended to be used elsewhere
#endif /* P_INTERNAL_GUARD__ */

#include "core/int.h"
#include "../window.h"
#define P_INTERNAL_GUARD__
#include "window-fb.h"
#include "window-x11.h"
#undef P_INTERNAL_GUARD__

#define N_WINDOW_TYPES 2
enum window_type {
    WINDOW_TYPE_X11,
    WINDOW_TYPE_FRAMEBUFFER,
};

struct p_window {
    enum window_type type;

    i32 x, y, w, h;

    union {
        struct window_x11 x11;
        struct window_fb fb;
    };

    enum p_window_color_type color_type;
};


#endif /* WINDOW_INTERNAL_H_ */
