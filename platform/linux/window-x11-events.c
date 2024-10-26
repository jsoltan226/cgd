#define _GNU_SOURCE
#include "../event.h"
#include "../mouse.h"
#include "../keyboard.h"
#include <core/log.h>
#include <core/int.h>
#include <core/math.h>
#include <core/util.h>
#include <stdlib.h>
#include <string.h>
#include <stdatomic.h>
#include <xcb/xcb.h>
#include <xcb/xproto.h>
#include <xcb/xinput.h>
#define P_INTERNAL_GUARD__
#include "window-x11-events.h"
#undef P_INTERNAL_GUARD__
#define P_INTERNAL_GUARD__
#include "window-x11.h"
#undef P_INTERNAL_GUARD__
#define P_INTERNAL_GUARD__
#include "libxcb-rtld.h"
#undef P_INTERNAL_GUARD__
#define P_INTERNAL_GUARD__
#include "mouse-x11.h"
#undef P_INTERNAL_GUARD__
#define P_INTERNAL_GUARD__
#include "keyboard-x11.h"
#undef P_INTERNAL_GUARD__

#define MODULE_NAME "window-x11-events"

static void handle_event(struct window_x11 *win, xcb_generic_event_t *ev);
static void handle_ge_event(struct window_x11 *win, xcb_ge_event_t *ev);

void * window_X11_event_listener_fn(void *arg)
{
    struct window_x11 *win = (struct window_x11 *)arg;

    while (atomic_load(&win->listener.running))
        handle_event(win, win->xcb.xcb_wait_for_event(win->conn));

    pthread_exit(NULL);
}

static void handle_event(struct window_x11 *win, xcb_generic_event_t *ev)
{
    switch (ev->response_type & ~0x80) {
    case XCB_CLIENT_MESSAGE: {
        xcb_client_message_event_t *cm = (xcb_client_message_event_t *)ev;
        if (cm->data.data32[0] == win->WM_DELETE_WINDOW)
            p_event_send(&(struct p_event) { .type = P_EVENT_QUIT });

        break;
    }
    case XCB_DESTROY_NOTIFY:
        p_event_send(&(struct p_event) { .type = P_EVENT_QUIT });
        break;
    case XCB_GE_GENERIC:
        handle_ge_event(win, (xcb_ge_event_t *)ev);
        break;
    case XCB_EXPOSE:
        break;
    default:
        break;
    }

    u_nzfree(&ev);
}

static void handle_ge_event(struct window_x11 *win, xcb_ge_event_t *ge_ev)
{
    const union {
        xcb_input_key_press_event_t *key_press;
        xcb_input_key_press_event_t *key_release;
        xcb_input_button_press_event_t *button_press;
        xcb_input_button_release_event_t *button_release;
        xcb_input_motion_event_t *motion;
    } ev = { (void *)ge_ev };

    /* Used in mouse event cases */
    struct mouse_x11_atomic_rw *mouse_rw;
    u32 button_bits;

    switch(ge_ev->event_type) {
    case XCB_INPUT_KEY_PRESS:
        if (ev.key_press->deviceid != win->master_keyboard_id)
            break;
        if (!win->registered_keyboard)
            break;

        const xcb_keysym_t press_keysym = win->xcb.xcb_key_symbols_get_keysym(
            win->key_symbols,
            ev.key_press->detail,
            0
        );
        X11_keyboard_store_key_event(
            win->registered_keyboard->key_events,
            press_keysym,
            KEYBOARD_X11_PRESS
        );
        break;
    case XCB_INPUT_KEY_RELEASE:
        if (ev.key_release->deviceid != win->master_keyboard_id)
            break;
        if (!win->registered_keyboard)
            break;

        const xcb_keysym_t release_keysym = win->xcb.xcb_key_symbols_get_keysym(
            win->key_symbols,
            ev.key_release->detail,
            0
        );
        X11_keyboard_store_key_event(
            win->registered_keyboard->key_events,
            release_keysym,
            KEYBOARD_X11_RELEASE
        );
        break;
    case XCB_INPUT_BUTTON_PRESS:
        if (ev.button_press->deviceid != win->master_mouse_id)
            break;

        if (!win->registered_mouse) break;
        mouse_rw = (struct mouse_x11_atomic_rw *)
            &win->registered_mouse->atomic_mouse;

        button_bits = atomic_load(&mouse_rw->button_bits);

        if (ev.button_press->detail == 1)
            button_bits |= P_MOUSE_LEFTBUTTONMASK;
        if (ev.button_press->detail == 2)
            button_bits |= P_MOUSE_MIDDLEBUTTONMASK;
        if (ev.button_press->detail == 3)
            button_bits |= P_MOUSE_RIGHTBUTTONMASK;
  
        atomic_store(&mouse_rw->button_bits, button_bits);

        break;
    case XCB_INPUT_BUTTON_RELEASE:
        if (ev.button_release->deviceid != win->master_mouse_id)
            break;

        if (!win->registered_mouse) break;
        mouse_rw = (struct mouse_x11_atomic_rw *)
            &win->registered_mouse->atomic_mouse;

        button_bits = atomic_load(&mouse_rw->button_bits);

        if (ev.button_release->detail == 1)
            button_bits &= ~P_MOUSE_LEFTBUTTONMASK;
        if (ev.button_release->detail == 2)
            button_bits &= ~P_MOUSE_MIDDLEBUTTONMASK;
        if (ev.button_release->detail == 3)
            button_bits &= ~P_MOUSE_RIGHTBUTTONMASK;
  
        atomic_store(&mouse_rw->button_bits, button_bits);

        break;
    case XCB_INPUT_MOTION:
        if (ev.button_release->deviceid != win->master_mouse_id)
            break;

        if (win->registered_mouse == NULL) break;
        mouse_rw = (struct mouse_x11_atomic_rw *)
            &win->registered_mouse->atomic_mouse;

        atomic_store(&mouse_rw->x, u_fp1616_to_f32(ev.motion->event_x));
        atomic_store(&mouse_rw->y, u_fp1616_to_f32(ev.motion->event_y));

        break;
    default:
        break;
    }
}
