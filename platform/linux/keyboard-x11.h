#ifndef KEYBOARD_X11_H_
#define KEYBOARD_X11_H_

#include "core/util.h"
#ifndef P_INTERNAL_GUARD__
#error This header file is internal to the cgd platform module and is not intended to be used elsewhere
#endif /* P_INTERNAL_GUARD__ */

#include "../keyboard.h"
#include <core/int.h>
#include <core/pressable-obj.h>
#include <xcb/xproto.h>
#include <xcb/xcb_keysyms.h>
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
    xcb_key_symbols_t *key_symbols;
};

i32 X11_keyboard_init(struct keyboard_x11 *kb, struct window_x11 *win);

void X11_keyboard_update_all_keys(struct keyboard_x11 *kb,
    pressable_obj_t pobj[P_KEYBOARD_N_KEYS]);

void X11_keyboard_destroy(struct keyboard_x11 *kb);

void X11_keyboard_store_key_event(
    volatile _Atomic i8 events[P_KEYBOARD_N_KEYS],
    xcb_keysym_t keysym, enum keyboard_x11_key_event event
);

static const i32 keycode_map[P_KEYBOARD_N_KEYS][2] = {
    { 0xff0d,   KB_KEYCODE_ENTER },
    { 0x0020,   KB_KEYCODE_SPACE },
    { 0xff1b,   KB_KEYCODE_ESCAPE },
    { 0xff52,   KB_KEYCODE_ARROWUP },
    { 0xff54,   KB_KEYCODE_ARROWDOWN },
    { 0xff51,   KB_KEYCODE_ARROWLEFT },
    { 0xff53,   KB_KEYCODE_ARROWRIGHT },
    { '0',      KB_KEYCODE_DIGIT0 },
    { '1',      KB_KEYCODE_DIGIT1 },
    { '2',      KB_KEYCODE_DIGIT2 },
    { '3',      KB_KEYCODE_DIGIT3 },
    { '4',      KB_KEYCODE_DIGIT4 },
    { '5',      KB_KEYCODE_DIGIT5 },
    { '6',      KB_KEYCODE_DIGIT6 },
    { '7',      KB_KEYCODE_DIGIT7 },
    { '8',      KB_KEYCODE_DIGIT8 },
    { '9',      KB_KEYCODE_DIGIT9 },
    { 'a',      KB_KEYCODE_A },
    { 'b',      KB_KEYCODE_B },
    { 'c',      KB_KEYCODE_C },
    { 'd',      KB_KEYCODE_D },
    { 'e',      KB_KEYCODE_E },
    { 'f',      KB_KEYCODE_F },
    { 'g',      KB_KEYCODE_G },
    { 'h',      KB_KEYCODE_H },
    { 'i',      KB_KEYCODE_I },
    { 'j',      KB_KEYCODE_J },
    { 'k',      KB_KEYCODE_K },
    { 'l',      KB_KEYCODE_L },
    { 'm',      KB_KEYCODE_M },
    { 'n',      KB_KEYCODE_N },
    { 'o',      KB_KEYCODE_O },
    { 'p',      KB_KEYCODE_P },
    { 'q',      KB_KEYCODE_Q },
    { 'r',      KB_KEYCODE_R },
    { 's',      KB_KEYCODE_S },
    { 't',      KB_KEYCODE_T },
    { 'u',      KB_KEYCODE_U },
    { 'v',      KB_KEYCODE_V },
    { 'w',      KB_KEYCODE_W },
    { 'x',      KB_KEYCODE_X },
    { 'y',      KB_KEYCODE_Y },
    { 'z',      KB_KEYCODE_Z },
};

#endif /* KEYBOARD_X11_H_ */
