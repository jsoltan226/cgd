#ifndef P_WINDOW_X11_H_
#define P_WINDOW_X11_H_

#include <xcb/xcb_image.h>
#ifndef P_INTERNAL_GUARD__
#error This header file is internal to the cgd platform module and is not intended to be used elsewhere
#endif /* P_INTERNAL_GUARD__ */

#include <core/int.h>
#include <core/pixel.h>
#include <core/shapes.h>
#include <stdbool.h>
#include <pthread.h>
#include <xcb/xcb.h>
#include <xcb/xproto.h>

#define P_INTERNAL_GUARD__
#include "libx11-rtld.h"
#undef P_INTERNAL_GUARD__

struct window_x11 {
    bool exists; /* Sanity check to avoid double-frees */

    xcb_connection_t *conn;
    const xcb_setup_t *setup;
    xcb_screen_t *screen;
    xcb_screen_iterator_t iter;
    xcb_window_t win;

    xcb_atom_t UTF8_STRING;
    xcb_atom_t NET_WM_NAME;
    xcb_atom_t NET_WM_STATE_ABOVE;
    xcb_atom_t WM_PROTOCOLS;
    xcb_atom_t WM_DELETE_WINDOW;

    xcb_gcontext_t gc;
    xcb_image_t *xcb_image;
    struct pixel_flat_data *fb;

    struct window_x11_listener {
        bool running;
        pthread_t thread;
    } listener;
};

/* Returns 0 on success and non-zero on failure.
 * Does not clean up if an error happens */
/* Does not perform any parameter validation! */
i32 window_X11_open(struct window_x11 *x11,
    const unsigned char *title, const rect_t *area, const u32 flags);

/* Also unloads libX11 if there are no open windows left */
void window_X11_close(struct window_x11 *x11);

/* Does not perform any parameter validation! */
void window_X11_render(struct window_x11 *x11);

void window_X11_bind_fb(struct window_x11 *x11, struct pixel_flat_data *fb);
void window_X11_unbind_fb(struct window_x11 *win);

#endif /* P_WINDOW_X11_H_ */
