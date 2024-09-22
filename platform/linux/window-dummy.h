#ifndef WINDOW_DUMMY_H_
#define WINDOW_DUMMY_H_

#ifndef P_INTERNAL_GUARD__
#error This header file is internal to the cgd platform module and is not intended to be used elsewhere
#endif /* P_INTERNAL_GUARD__ */

#include "core/int.h"
#include "../window.h"
#define P_INTERNAL_GUARD__
#include "window-x11.h"
#undef P_INTERNAL_GUARD__

struct window_dummy {
    bool is_X11;
    struct window_x11 x11;
};

i32 dummy_window_init(struct window_dummy *win, bool is_x11);
void dummy_window_destroy(struct window_dummy *win);

#endif /* WINDOW_DUMMY_H_ */
