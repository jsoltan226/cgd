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
#define P_INTERNAL_GUARD__
#include "window-x11-present-sw.h"
#undef P_INTERNAL_GUARD__

struct x11_render_egl_ctx {
    bool initialized_;
};

struct window_x11 {
    bool exists; /* Sanity check to avoid double-frees */

    /* The interface to the generic `p_window` API;
     * holds basic information like window position and dimensions,
     * pixel format and feature (e.g. vsync) support.
     * Although the actual struct is stored somewhere else,
     * we are responsible for initializing it. */
    struct p_window_info *generic_info_p;

    /* The libxcb-* library functions, loaded at runtime with `dlopen` */
    struct libxcb xcb;

    /* The libxcb "context" that represents the connection to the server */
    xcb_connection_t *conn;

    /* The handle to the X window */
    xcb_window_t win_handle;

    /* Information about the screen on which our window is displayed */
    xcb_screen_t *screen;

    /* These are the standard X11 Atoms ("additional window properties")
     * that are read and/or set during window initialization */
    struct window_x11_atoms {
        xcb_atom_t UTF8_STRING;
        xcb_atom_t NET_WM_NAME;
        xcb_atom_t NET_WM_STATE_ABOVE;
        xcb_atom_t WM_PROTOCOLS;
        xcb_atom_t WM_DELETE_WINDOW;
    } atoms;

    /* Anything related to frame presentation.
     * Which one of these is used depends on the window gpu acceleration
     * (`generic_info_p->gpu_acceleration`). */
    union window_x11_render_ctx {
        struct x11_render_software_ctx sw; /* Used with software rendering */
        struct x11_render_egl_ctx egl; /* Used with OpenGL/EGL rendering */
    } render;

    /* Stuff related to the event listener thread */
    struct window_x11_listener {
        _Atomic bool running;
        p_mt_thread_t thread;
    } listener;

    /* Anything used to process input events */
    struct window_x11_input {
        /* Cached data of the Xinput2 extension, which we use to
         * receive keyboard and mouse input from the server */
        const struct xcb_query_extension_reply_t *xinput_ext_data;

        /* The keyboard that will be receiving any keyboard input events.
         * Used to interface with `p_keyboard`. */
        struct keyboard_x11 *registered_keyboard;
        /* These are used to notify the listener thread that the keyboard
         * is no longer usable and events shouldn't be written to it */
        _Atomic bool keyboard_deregistration_notify;
        p_mt_cond_t keyboard_deregistration_ack;

        /* The mouse that will be receiving any mouse input events.
         * Used to interface with `p_mouse`. */
        struct mouse_x11 *registered_mouse;
        /* These are used to notify the listener thread that the mouse
         * is no longer usable and events shouldn't be written to it */
        _Atomic bool mouse_deregistration_notify;
        p_mt_cond_t mouse_deregistration_ack;

        /* The handles to the "master" X11 devices - virtual devices
         * that combine input from all physical ("slave") devices */
        xcb_input_device_id_t master_mouse_id, master_keyboard_id;

        /* Used for easy translation of X11 keyboard events
         * to keyboard symbols, because the raw event codes are useless */
        xcb_key_symbols_t *key_symbols;
    } input;
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

i32 X11_check_xinput2_extension(xcb_connection_t *conn,
    const struct libxcb *xcb);
i32 X11_check_shm_extension(xcb_connection_t *conn,
    const struct libxcb *xcb);
i32 X11_check_present_extension(xcb_connection_t *conn,
    const struct libxcb *xcb);

#endif /* P_WINDOW_X11_H_ */
