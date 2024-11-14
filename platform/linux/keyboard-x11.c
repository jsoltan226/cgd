#include "../keyboard.h"
#include "../thread.h"
#include "core/log.h"
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

    kb->unprocessed_ev.mutex = p_mt_mutex_create();
    for (u32 i = 0; i < P_KEYBOARD_N_KEYS; i++) {
        kb->unprocessed_ev.queue[i] = vector_new(i8);
    }
    atomic_store(&kb->unprocessed_ev.empty, false);

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

    bool queue_empty = true;
    if (!atomic_load(&kb->unprocessed_ev.empty)) {
        p_mt_mutex_lock(&kb->unprocessed_ev.mutex);
        
        for (u32 i = 0; i < P_KEYBOARD_N_KEYS; i++) {
            if (vector_size(kb->unprocessed_ev.queue[i]) <= 0)
                continue; /* Skip this key if it's queue is empty */
            else if (vector_size(kb->unprocessed_ev.queue[i]) > 1)
                queue_empty = false;

            process_ev(kb->unprocessed_ev.queue[i][0], &pobjs[i]);
            vector_erase(kb->unprocessed_ev.queue[i], 0);
        }

        p_mt_mutex_unlock(&kb->unprocessed_ev.mutex);
    }

    /* If any one of the keys' queue is not yet empty,
     * this variable will be set to false */
    atomic_store(&kb->unprocessed_ev.empty, queue_empty);
}

void X11_keyboard_destroy(struct keyboard_x11 *kb)
{
    if (kb == NULL) return;

    window_X11_deregister_keyboard(kb->win);

    p_mt_mutex_destroy(&kb->unprocessed_ev.mutex);
    for (u32 i = 0; i < P_KEYBOARD_N_KEYS; i++) {
        vector_destroy(&kb->unprocessed_ev.queue[i]);
    }

    memset(kb, 0, sizeof(struct keyboard_x11));
}

void X11_keyboard_store_key_event(struct keyboard_x11 *kb,
    xcb_keysym_t keysym, enum keyboard_x11_key_event event)
{
    enum p_keyboard_keycode p_kb_keycode = -1;
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
    if (p_kb_keycode == -1) return;

    /* If there is an unprocessed event for this key,
     * push the current one to the queue */
    if (atomic_load(&kb->key_events[p_kb_keycode]) != KEYBOARD_X11_NOTHING) {
        p_mt_mutex_lock(&kb->unprocessed_ev.mutex);
        vector_push_back(kb->unprocessed_ev.queue[p_kb_keycode], event);
        atomic_store(&kb->unprocessed_ev.empty, false);
        p_mt_mutex_unlock(&kb->unprocessed_ev.mutex);
    } else {
        atomic_store(&kb->key_events[p_kb_keycode], event);
    }
}
