#include "../keyboard.h"
#include <core/int.h>
#include <core/util.h>
#include <core/vector.h>
#include <core/pressable-obj.h>
#include <string.h>
#include <stdatomic.h>

#define P_INTERNAL_GUARD__
#include "keyboard-x11.h"
#undef P_INTERNAL_GUARD__
#define P_INTERNAL_GUARD__
#include "window-x11.h"
#undef P_INTERNAL_GUARD__

#define MODULE_NAME "keyboard-x11"

static inline void process_ev(const i8 ev, pressable_obj_t *pobj)
{
    if (ev == KEYBOARD_X11_PRESS)
        pressable_obj_update(pobj, true);
    else if (ev == KEYBOARD_X11_RELEASE)
        pressable_obj_update(pobj, false);
    else if (pobj->pressed || pobj->up)
        pressable_obj_update(pobj, pobj->pressed);
}

i32 X11_keyboard_init(struct keyboard_x11 *kb, struct window_x11 *win)
{
    u_check_params(win != NULL);
    memset(kb, 0, sizeof(struct keyboard_x11));

    kb->win = win;

    if (window_X11_register_keyboard(win, kb))
        goto_error("Failed to register the keyboard");

    return 0;

err:
    X11_keyboard_destroy(kb);
    return 1;
}

void X11_keyboard_update_all_keys(struct keyboard_x11 *kb,
    pressable_obj_t pobjs[P_KEYBOARD_N_KEYS])
{
    for (u32 i = 0; i < P_KEYBOARD_N_KEYS; i++) {
        /* Nothing really happens even if there's a data race here */
        process_ev(atomic_load(&kb->key_events[i]), &pobjs[i]);
        atomic_store(&kb->key_events[i], KEYBOARD_X11_NOTHING);
    }
}

void X11_keyboard_destroy(struct keyboard_x11 *kb)
{
    if (kb == NULL) return;

    window_X11_deregister_keyboard(kb->win);

    memset(kb, 0, sizeof(struct keyboard_x11));
}

void X11_keyboard_store_key_event(struct keyboard_x11 *kb,
    xcb_keysym_t keysym, enum keyboard_x11_key_event event)
{
    enum p_keyboard_keycode p_kb_keycode = P_KEYBOARD_FAIL_;
    if (keysym >= '0' && keysym < '9')
        p_kb_keycode = keysym - '0' + KB_KEYCODE_DIGIT0;
    else if (keysym >= 'a' && keysym <= 'z')
        p_kb_keycode = keysym - 'a' + KB_KEYCODE_A;
    else {
        for (u32 i = 0; i < P_KEYBOARD_N_KEYS; i++) {
            if (keycode_map[i] == keysym) {
                p_kb_keycode = i;
                break;
            }
        }
    }
    if (p_kb_keycode == P_KEYBOARD_FAIL_) return;

    atomic_store(&kb->key_events[p_kb_keycode], event);
}
