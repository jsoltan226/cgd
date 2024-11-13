#ifndef MOUSE_X11_H_
#define MOUSE_X11_H_

#ifndef P_INTERNAL_GUARD__
#error This header file is internal to the cgd platform module and is not intended to be used elsewhere
#endif /* P_INTERNAL_GUARD__ */

#include "../mouse.h"
#include <core/int.h>
#define P_INTERNAL_GUARD__
#include "window-x11.h"
#undef P_INTERNAL_GUARD__

struct mouse_x11 {
    struct window_x11 *win;
    volatile _Atomic u32 button_bits;
    volatile _Atomic f32 x, y;
};

i32 mouse_X11_init(struct mouse_x11 *mouse, struct window_x11 *win);

void mouse_X11_update(struct p_mouse *mouse);

void mouse_X11_destroy(struct mouse_x11 *mouse);

#endif /* MOUSE_X11_H_ */
