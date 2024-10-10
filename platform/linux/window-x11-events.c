#include <xcb/xcb.h>
#include <xcb/xinput.h>
#include <xcb/xproto.h>
#define _GNU_SOURCE
#include "../event.h"
#include "../mouse.h"
#include <core/log.h>
#include <core/math.h>
#include <stdlib.h>
#include <string.h>
#include <stdatomic.h>
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

    free(ev);
}

static void handle_ge_event(struct window_x11 *win, xcb_ge_event_t *ev)
{
    switch(ev->event_type) {
    case XCB_INPUT_BUTTON_PRESS: {
        if (!win->registered_mouse) break;

        xcb_input_button_press_event_t *bev =
            (xcb_input_button_press_event_t *)ev;
        struct mouse_x11_atomic_rw *mouse = (struct mouse_x11_atomic_rw *)
            &win->registered_mouse->atomic_mouse;

        u32 button_bits = atomic_load(&mouse->button_bits);

        if (bev->detail == 1)
            button_bits |= P_MOUSE_LEFTBUTTONMASK;
        if (bev->detail == 2)
            button_bits |= P_MOUSE_MIDDLEBUTTONMASK;
        if (bev->detail == 3)
            button_bits |= P_MOUSE_RIGHTBUTTONMASK;
  
        atomic_store(&mouse->button_bits, button_bits);

        break;
    }
    case XCB_INPUT_BUTTON_RELEASE: {
        if (!win->registered_mouse) break;

        xcb_input_button_release_event_t *bev =
            (xcb_input_button_release_event_t *)ev;
        struct mouse_x11_atomic_rw *mouse = (struct mouse_x11_atomic_rw *)
            &win->registered_mouse->atomic_mouse;

        u32 button_bits = atomic_load(&mouse->button_bits);

        if (bev->detail == 1)
            button_bits &= ~P_MOUSE_LEFTBUTTONMASK;
        if (bev->detail == 2)
            button_bits &= ~P_MOUSE_MIDDLEBUTTONMASK;
        if (bev->detail == 3)
            button_bits &= ~P_MOUSE_RIGHTBUTTONMASK;
  
        atomic_store(&mouse->button_bits, button_bits);

        break;
    }
    case XCB_INPUT_MOTION: {
        if (!win->registered_mouse) break;

        xcb_input_motion_event_t *mev = (xcb_input_motion_event_t *)ev;

        struct mouse_x11_atomic_rw *mouse = (struct mouse_x11_atomic_rw *)
            &win->registered_mouse->atomic_mouse;

        const i32 x = (i32)u_fp1616_to_f32(mev->event_x);
        const i32 y = (i32)u_fp1616_to_f32(mev->event_y);
        atomic_store(&mouse->x, x);
        atomic_store(&mouse->y, y);

        break;
    }
    default:
        break;
    }
}
