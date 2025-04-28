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

struct x11_render_software_ctx {
    bool initialized_;
    xcb_gcontext_t window_gc;

    bool use_shm;
    struct x11_render_software_buf {
        bool initialized_;
        bool is_shm;
        union {
            struct x11_render_software_malloced_buf {
                xcb_image_t *image;
            } malloced;
            struct x11_render_software_shm_buf {
                xcb_gcontext_t gc;
                xcb_pixmap_t pixmap;
                xcb_shm_segment_info_t shm_info;
                u32 root_depth;
            } shm;
        };
        struct pixel_flat_data user_ret;
    } buffers[2], *curr_front_buf, *curr_back_buf;
};
struct x11_render_egl_ctx {
    bool initialized_;
};

struct window_x11 {
    bool exists; /* Sanity check to avoid double-frees */

    struct libxcb xcb;

    xcb_connection_t *conn;
    const xcb_setup_t *setup;
    xcb_screen_t *screen;
    xcb_screen_iterator_t iter;
    xcb_window_t win;
    bool win_created;

    struct p_window_info *generic_info_p;

    union window_x11_render_ctx {
        struct x11_render_software_ctx sw;
        struct x11_render_egl_ctx egl;
    } render;

    xcb_atom_t UTF8_STRING;
    xcb_atom_t NET_WM_NAME;
    xcb_atom_t NET_WM_STATE_ABOVE;
    xcb_atom_t WM_PROTOCOLS;
    xcb_atom_t WM_DELETE_WINDOW;

    struct window_x11_listener {
        _Atomic bool running;
        p_mt_thread_t thread;
    } listener;

    struct keyboard_x11 *registered_keyboard;
    _Atomic bool keyboard_deregistration_notify;
    p_mt_cond_t keyboard_deregistration_ack;

    struct mouse_x11 *registered_mouse;
    _Atomic bool mouse_deregistration_notify;
    p_mt_cond_t mouse_deregistration_ack;

    xcb_input_device_id_t master_mouse_id, master_keyboard_id;

    xcb_key_symbols_t *key_symbols;
};

/* Returns 0 on success and non-zero on failure.
 * Does not clean up if an error happens */
/* Does not perform any parameter validation! */
i32 window_X11_open(struct window_x11 *win, struct p_window_info *info,
    const char *title, const rect_t *area, const u32 flags);

/* Also unloads libX11 if there are no open windows left */
void window_X11_close(struct window_x11 *x11);

/* Does not perform any parameter validation! */
struct pixel_flat_data * window_X11_swap_buffers(struct window_x11 *win,
    enum p_window_present_mode present_mode);

i32 window_X11_register_keyboard(struct window_x11 *win,
    struct keyboard_x11 *kb);
i32 window_X11_register_mouse(struct window_x11 *win,
    struct mouse_x11 *mouse);

void window_X11_deregister_keyboard(struct window_x11 *win);
void window_X11_deregister_mouse(struct window_x11 *win);

i32 window_X11_set_acceleration(struct window_x11 *win,
    enum p_window_acceleration new_val);

#endif /* P_WINDOW_X11_H_ */
