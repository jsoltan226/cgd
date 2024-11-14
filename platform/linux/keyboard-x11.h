#ifndef KEYBOARD_X11_H_
#define KEYBOARD_X11_H_

#ifndef P_INTERNAL_GUARD__
#error This header file is internal to the cgd platform module and is not intended to be used elsewhere
#endif /* P_INTERNAL_GUARD__ */

#include "../keyboard.h"
#include "../thread.h"
#include <core/int.h>
#include <core/vector.h>
#include <core/pressable-obj.h>
#include <stdatomic.h>
#include <xcb/xproto.h>
#define P_INTERNAL_GUARD__
#include "window-x11.h"
#undef P_INTERNAL_GUARD__

enum keyboard_x11_key_event {
    KEYBOARD_X11_NOTHING = 0,
    KEYBOARD_X11_PRESS = 1,
    KEYBOARD_X11_RELEASE = -1,
};

struct keyboard_x11 {
    struct window_x11 *win;
    _Atomic volatile i8 key_events[P_KEYBOARD_N_KEYS];

    struct {
        VECTOR(i8) queue[P_KEYBOARD_N_KEYS];
        p_mt_mutex_t mutex;
        _Atomic bool empty;
    } unprocessed_ev;
};

i32 X11_keyboard_init(struct keyboard_x11 *kb, struct window_x11 *win);

void X11_keyboard_update_all_keys(struct keyboard_x11 *kb,
    pressable_obj_t pobj[P_KEYBOARD_N_KEYS]);

void X11_keyboard_destroy(struct keyboard_x11 *kb);

void X11_keyboard_store_key_event(struct keyboard_x11 *kb,
    xcb_keysym_t keysym, enum keyboard_x11_key_event event);

static const i32 keycode_map[P_KEYBOARD_N_KEYS] = {
    [KB_KEYCODE_ENTER]      = 0xff0d,
    [KB_KEYCODE_SPACE]      = 0x0020,
    [KB_KEYCODE_ESCAPE]     = 0xff1b,
    [KB_KEYCODE_ARROWUP]    = 0xff52,
    [KB_KEYCODE_ARROWDOWN]  = 0xff54,
    [KB_KEYCODE_ARROWLEFT]  = 0xff51,
    [KB_KEYCODE_ARROWRIGHT] = 0xff53,
    [KB_KEYCODE_DIGIT0]     = '0',
    [KB_KEYCODE_DIGIT1]     = '1',
    [KB_KEYCODE_DIGIT2]     = '2',
    [KB_KEYCODE_DIGIT3]     = '3',
    [KB_KEYCODE_DIGIT4]     = '4',
    [KB_KEYCODE_DIGIT5]     = '5',
    [KB_KEYCODE_DIGIT6]     = '6',
    [KB_KEYCODE_DIGIT7]     = '7',
    [KB_KEYCODE_DIGIT8]     = '8',
    [KB_KEYCODE_DIGIT9]     = '9',
    [KB_KEYCODE_A]          = 'a',
    [KB_KEYCODE_B]          = 'b',
    [KB_KEYCODE_C]          = 'c',
    [KB_KEYCODE_D]          = 'd',
    [KB_KEYCODE_E]          = 'e',
    [KB_KEYCODE_F]          = 'f',
    [KB_KEYCODE_G]          = 'g',
    [KB_KEYCODE_H]          = 'h',
    [KB_KEYCODE_I]          = 'i',
    [KB_KEYCODE_J]          = 'j',
    [KB_KEYCODE_K]          = 'k',
    [KB_KEYCODE_L]          = 'l',
    [KB_KEYCODE_M]          = 'm',
    [KB_KEYCODE_N]          = 'n',
    [KB_KEYCODE_O]          = 'o',
    [KB_KEYCODE_P]          = 'p',
    [KB_KEYCODE_Q]          = 'q',
    [KB_KEYCODE_R]          = 'r',
    [KB_KEYCODE_S]          = 's',
    [KB_KEYCODE_T]          = 't',
    [KB_KEYCODE_U]          = 'u',
    [KB_KEYCODE_V]          = 'v',
    [KB_KEYCODE_W]          = 'w',
    [KB_KEYCODE_X]          = 'x',
    [KB_KEYCODE_Y]          = 'y',
    [KB_KEYCODE_Z]          = 'z',
};

#endif /* KEYBOARD_X11_H_ */
