#include "../window.h"
#include "core/util.h"
#include "core/int.h"
#include <pthread.h>
#define P_INTERNAL_GUARD__
#include "window-x11.h"
#undef P_INTERNAL_GUARD__
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
