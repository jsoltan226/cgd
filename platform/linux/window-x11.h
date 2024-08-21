#ifndef P_WINDOW_X11_H_
#define P_WINDOW_X11_H_

#ifndef P_INTERNAL_GUARD__
#error This header file is internal to the cgd platform module and is not intended to be used elsewhere
#endif /* P_INTERNAL_GUARD__ */

#include <X11/X.h>
#include <X11/Xlib.h>
#include "core/int.h"

struct window_x11 {
    Display *dpy;
    Window root, win;
    i32 scr;
};

i32 window_X11_open(struct window_x11 *x11, i32 x, i32 y, i32 w, i32 h);
void window_X11_close(struct window_x11 *x11);

#endif /* P_WINDOW_X11_H_ */
