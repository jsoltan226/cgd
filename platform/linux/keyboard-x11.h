#ifndef KEYBOARD_X11_H_
#define KEYBOARD_X11_H_

#ifndef P_INTERNAL_GUARD__
#error This header file is internal to the cgd platform module and is not intended to be used elsewhere
#endif /* P_INTERNAL_GUARD__ */

#include "../keyboard.h"
#include "core/int.h"
#include "core/pressable-obj.h"
#include <X11/Xlib.h>
#define P_INTERNAL_GUARD__
#include "window-x11.h"
#undef P_INTERNAL_GUARD__
#define P_INTERNAL_GUARD__
#include "libx11_rtld.h"
#undef P_INTERNAL_GUARD__

struct keyboard_x11 {
    Display *dpy;
    struct libX11 Xlib;
};

i32 keyboard_X11_init(struct keyboard_x11 *kb, struct window_x11 *win);

void keyboard_X11_update_all_keys(struct keyboard_x11 *kb,
    pressable_obj_t pobj[P_KEYBOARD_N_KEYS]);

void keyboard_X11_destroy(struct keyboard_x11 *kb);

static const i32 x11_keycode_2_p_keycode_map[P_KEYBOARD_N_KEYS][2] = {
    { XK_Return,    KB_KEYCODE_ENTER },
    { XK_space,     KB_KEYCODE_SPACE },
    { XK_e,         KB_KEYCODE_ESCAPE },
    { XK_0,         KB_KEYCODE_DIGIT0 },
    { XK_1,         KB_KEYCODE_DIGIT1 },
    { XK_2,         KB_KEYCODE_DIGIT2 },
    { XK_3,         KB_KEYCODE_DIGIT3 },
    { XK_4,         KB_KEYCODE_DIGIT4 },
    { XK_5,         KB_KEYCODE_DIGIT5 },
    { XK_6,         KB_KEYCODE_DIGIT6 },
    { XK_7,         KB_KEYCODE_DIGIT7 },
    { XK_8,         KB_KEYCODE_DIGIT8 },
    { XK_9,         KB_KEYCODE_DIGIT9 },
    { XK_a,         KB_KEYCODE_A },
    { XK_b,         KB_KEYCODE_B },
    { XK_c,         KB_KEYCODE_C },
    { XK_d,         KB_KEYCODE_D },
    { XK_e,         KB_KEYCODE_E },
    { XK_f,         KB_KEYCODE_F },
    { XK_g,         KB_KEYCODE_G },
    { XK_h,         KB_KEYCODE_H },
    { XK_i,         KB_KEYCODE_I },
    { XK_j,         KB_KEYCODE_J },
    { XK_k,         KB_KEYCODE_K },
    { XK_l,         KB_KEYCODE_L },
    { XK_m,         KB_KEYCODE_M },
    { XK_n,         KB_KEYCODE_N },
    { XK_o,         KB_KEYCODE_O },
    { XK_p,         KB_KEYCODE_P },
    { XK_q,         KB_KEYCODE_Q },
    { XK_r,         KB_KEYCODE_R },
    { XK_s,         KB_KEYCODE_S },
    { XK_t,         KB_KEYCODE_T },
    { XK_u,         KB_KEYCODE_U },
    { XK_v,         KB_KEYCODE_V },
    { XK_w,         KB_KEYCODE_W },
    { XK_x,         KB_KEYCODE_X },
    { XK_y,         KB_KEYCODE_Y },
    { XK_z,         KB_KEYCODE_Z },
    { XK_p,         KB_KEYCODE_ARROWUP },
    { XK_n,         KB_KEYCODE_ARROWDOWN },
    { XK_t,         KB_KEYCODE_ARROWLEFT },
    { XK_t,         KB_KEYCODE_ARROWRIGHT },
};

#endif /* KEYBOARD_X11_H_ */
