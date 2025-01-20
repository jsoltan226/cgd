#ifndef MOUSE_INTERNAL_H_
#define MOUSE_INTERNAL_H_

#ifndef P_INTERNAL_GUARD__
#error This header file is internal to the cgd platform module and is not intended to be used elsewhere
#endif /* P_INTERNAL_GUARD__ */

#define P_INTERNAL_GUARD__
#include "window-internal.h"
#undef P_INTERNAL_GUARD__
#define P_INTERNAL_GUARD__
#include "mouse-evdev.h"
#undef P_INTERNAL_GUARD__
#define P_INTERNAL_GUARD__
#include "mouse-x11.h"
#undef P_INTERNAL_GUARD__
#include <core/shapes.h>
#include <core/pressable-obj.h>

#define N_MOUSE_TYPES 3
#define MOUSE_TYPES_LIST    \
    X_(MOUSE_TYPE_FAIL)     \
    X_(MOUSE_TYPE_X11)      \
    X_(MOUSE_TYPE_EVDEV)    \

#define X_(name) name,
enum mouse_type {
    MOUSE_TYPES_LIST
};
#undef X_

#define X_(name) #name,
static const char * const mouse_type_strings[N_MOUSE_TYPES] = {
    MOUSE_TYPES_LIST
};
#undef X_

static const enum mouse_type
    mouse_fallback_modes[N_WINDOW_TYPES][N_MOUSE_TYPES] = {
    [WINDOW_TYPE_X11] = {
        MOUSE_TYPE_X11,
        MOUSE_TYPE_FAIL
    },
    [WINDOW_TYPE_DRI] = {
        MOUSE_TYPE_EVDEV,
        MOUSE_TYPE_FAIL
    },
    [WINDOW_TYPE_FBDEV] = {
        MOUSE_TYPE_EVDEV,
        MOUSE_TYPE_FAIL
    },
    [WINDOW_TYPE_DUMMY] = {
        MOUSE_TYPE_FAIL
    }
};

struct p_mouse {
    struct p_window *win;

    enum mouse_type type;
    union {
        struct mouse_x11 x11;
        struct mouse_evdev evdev;
    };

    pressable_obj_t buttons[P_MOUSE_N_BUTTONS];

    vec2d_t pos;

    bool is_out_of_window;
};

#endif /* MOUSE_INTERNAL_H_ */
