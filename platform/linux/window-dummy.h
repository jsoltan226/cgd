#ifndef WINDOW_DUMMY_H_
#define WINDOW_DUMMY_H_

#ifndef P_INTERNAL_GUARD__
#error This header file is internal to the cgd platform module and is not intended to be used elsewhere
#endif /* P_INTERNAL_GUARD__ */

#include "../window.h"
#include <core/int.h>
#define P_INTERNAL_GUARD__
#include "window-x11.h"
#undef P_INTERNAL_GUARD__

struct window_dummy {
};

void dummy_window_init(struct window_dummy *win);
void dummy_window_destroy(struct window_dummy *win);

#endif /* WINDOW_DUMMY_H_ */
