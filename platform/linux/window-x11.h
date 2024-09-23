#ifndef P_WINDOW_X11_H_
#define P_WINDOW_X11_H_

#ifndef P_INTERNAL_GUARD__
#error This header file is internal to the cgd platform module and is not intended to be used elsewhere
#endif /* P_INTERNAL_GUARD__ */

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "core/int.h"
#include "core/shapes.h"
#include "core/pixel.h"
#include <stdbool.h>

#define P_INTERNAL_GUARD__
#include "libx11_rtld.h"
#undef P_INTERNAL_GUARD__

struct window_x11 {
    Display *dpy; /* The connection to the X server */

    Window root; /* A handle to the root window (the "background") */
    Window win; /* A handle to our window */

    i32 scr; /* A handle to the screen */
    XVisualInfo vis_info; /* Screen visual info (colormap, bit depth, etc) */

    u32 screen_w, screen_h;

    GC gc; /* Graphics context. Used for graphics-related operations */

    /* The framebuffer (i.e. where the renderer writes raw pixel data) */
    struct pixel_flat_data data;

    /* A handle to an XImage bound to our framebuffer.
     * used for rendering with XPutImage */
    XImage *Ximg; 

    /* Used for detecting "window-close" events */
    Atom WM_DELETE_WINDOW;

    /* Used to tell whether an error occured in the call to XCreateWindow
     * or earlier. We need this to ensure we don't invoke XDestroyWindow
     * with an invalid window handle */
    bool bad_window;

    /* Whether the window has already been destroyed.
     * A safeguard against double-frees and stuff */
    bool closed;
};


/* Returns 0 on success and non-zero on failure.
 * Does not clean up if an error happens */
/* Does not perform any parameter validation! */
i32 window_X11_open(struct window_x11 *x11,
    const unsigned char *title, const rect_t *area, const u32 flags);

/* Also unloads libX11 if there are no open windows left */
void window_X11_close(struct window_x11 *x11);

/* Does not perform any parameter validation! */
void window_X11_render(struct window_x11 *x11,
    const pixel_t *data, const rect_t *area);

#endif /* P_WINDOW_X11_H_ */
