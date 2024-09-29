#define P_INTERNAL_GUARD__
#include "window-dummy.h"
#undef P_INTERNAL_GUARD__

#define MODULE_NAME "window-dummy"

void dummy_window_init(struct window_dummy *win)
{
}

void dummy_window_destroy(struct window_dummy *win)
{
}
