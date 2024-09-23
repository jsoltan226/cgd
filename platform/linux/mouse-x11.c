#include "core/int.h"
#include "../mouse.h"
#include <stdbool.h>
#define P_INTERNAL_GUARD__
#include "mouse-x11.h"
#undef P_INTERNAL_GUARD__
#define P_INTERNAL_GUARD__
#include "window-x11.h"
#undef P_INTERNAL_GUARD__

#define MODULE_NAME "mouse-x11"

i32 mouse_X11_init(struct mouse_x11 *mouse, struct window_x11 *win, u32 flags)
{
    return 1;
}

void mouse_X11_get_state(struct mouse_x11 *mouse,
    struct p_mouse_state *o, bool update)
{
}

void mouse_X11_destroy(struct mouse_x11 *mouse)
{
}
