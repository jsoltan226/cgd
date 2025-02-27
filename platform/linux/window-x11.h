#ifndef P_WINDOW_X11_H_
#define P_WINDOW_X11_H_

#ifndef P_INTERNAL_GUARD__
#error This header file is internal to the cgd platform module and is not intended to be used elsewhere
#endif /* P_INTERNAL_GUARD__ */

#include "../thread.h"
#include "../window.h"
#include <core/int.h>
#include <core/pixel.h>
#include <core/shapes.h>
#include <stdbool.h>
#include <xcb/xcb.h>
#include <xcb/xproto.h>
#include <xcb/xinput.h>
#include <xcb/xcb_image.h>
#include <xcb/xcb_keysyms.h>
#define P_INTERNAL_GUARD__
#include "libxcb-rtld.h"
#undef P_INTERNAL_GUARD__

struct window_x11 {
    bool exists; /* Sanity check to avoid double-frees */

    struct libxcb xcb;

    xcb_connection_t *conn;
    const xcb_setup_t *setup;
    xcb_screen_t *screen;
    xcb_screen_iterator_t iter;
    xcb_window_t win;
    bool win_created;

    bool shm_attached;
    xcb_shm_segment_info_t shm_info;

    xcb_atom_t UTF8_STRING;
    xcb_atom_t NET_WM_NAME;
    xcb_atom_t NET_WM_STATE_ABOVE;
    xcb_atom_t WM_PROTOCOLS;
    xcb_atom_t WM_DELETE_WINDOW;

    xcb_gcontext_t gc;
    xcb_image_t *xcb_image;

    struct window_x11_listener {
        _Atomic bool running;
        p_mt_thread_t thread;
    } listener;

    struct keyboard_x11 *registered_keyboard;
    _Atomic bool keyboard_deregistration_notify;
    p_mt_cond_t keyboard_deregistration_ack;

    struct mouse_x11 *registered_mouse;

    xcb_input_device_id_t master_mouse_id, master_keyboard_id;

    xcb_key_symbols_t *key_symbols;

    enum p_window_acceleration gpu_acceleration;
};

/* Returns 0 on success and non-zero on failure.
 * Does not clean up if an error happens */
/* Does not perform any parameter validation! */
i32 window_X11_open(struct window_x11 *x11,
    const unsigned char *title, const rect_t *area, const u32 flags);

/* Also unloads libX11 if there are no open windows left */
void window_X11_close(struct window_x11 *x11);

/* Does not perform any parameter validation! */
void window_X11_render(struct window_x11 *x11, struct pixel_flat_data *fb);

i32 window_X11_register_keyboard(struct window_x11 *win,
    struct keyboard_x11 *kb);
i32 window_X11_register_mouse(struct window_x11 *win,
    struct mouse_x11 *mouse);

void window_X11_deregister_keyboard(struct window_x11 *win);
void window_X11_deregister_mouse(struct window_x11 *win);

i32 window_X11_set_acceleration(struct window_x11 *win,
    enum p_window_acceleration new_val);

#endif /* P_WINDOW_X11_H_ */
