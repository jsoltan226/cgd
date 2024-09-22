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
    Display *dpy;
    Window root, win;
    i32 scr;

    XVisualInfo vis_info;

    u32 screen_w, screen_h;

    GC gc;
    struct pixel_flat_data data;
    XImage *Ximg;

    Atom WM_DELETE_WINDOW;

    bool bad_window;
    bool closed;
};

i32 window_X11_open(struct window_x11 *x11,
    const unsigned char *title, const rect_t *area, const u32 flags);

void window_X11_close(struct window_x11 *x11);

void window_X11_render(struct window_x11 *x11,
    const pixel_t *data, const rect_t *area);

#endif /* P_WINDOW_X11_H_ */
