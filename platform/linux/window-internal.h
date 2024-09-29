#ifndef WINDOW_INTERNAL_H_
#define WINDOW_INTERNAL_H_

#ifndef P_INTERNAL_GUARD__
#error This header file is internal to the cgd platform module and is not intended to be used elsewhere
#endif /* P_INTERNAL_GUARD__ */

#include "../window.h"
#include <core/int.h>
#define P_INTERNAL_GUARD__
#include "window-fb.h"
#undef P_INTERNAL_GUARD__
#define P_INTERNAL_GUARD__
#include "window-x11.h"
#undef P_INTERNAL_GUARD__
#define P_INTERNAL_GUARD__
#include "window-dummy.h"
#undef P_INTERNAL_GUARD__

#define N_WINDOW_TYPES 3
#define WINDOW_TYPE_LIST        \
    X_(WINDOW_TYPE_X11)         \
    X_(WINDOW_TYPE_FRAMEBUFFER) \
    X_(WINDOW_TYPE_DUMMY)       \

#define X_(name) name,
enum window_type {
    WINDOW_TYPE_LIST
};
#undef X_

#define X_(name) #name,
static const char * const window_type_strings[N_WINDOW_TYPES] = {
    WINDOW_TYPE_LIST
};
#undef X_

struct p_window {
    enum window_type type;

    i32 x, y, w, h;

    union {
        struct window_x11 x11;
        struct window_fb fb;
        struct window_dummy dummy;
    };

    enum p_window_color_type color_type;
};

#undef WINDOW_TYPE_LIST
#endif /* WINDOW_INTERNAL_H_ */
