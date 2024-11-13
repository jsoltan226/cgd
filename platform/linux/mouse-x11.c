#include "../mouse.h"
#include <core/int.h>
#include <core/pressable-obj.h>
#include <string.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <xcb/xinput.h>
#define P_INTERNAL_GUARD__
#include "mouse-x11.h"
#undef P_INTERNAL_GUARD__
#define P_INTERNAL_GUARD__
#include "window-x11.h"
#undef P_INTERNAL_GUARD__
#define P_INTERNAL_GUARD__
#include "mouse-internal.h"
#undef P_INTERNAL_GUARD__

#define MODULE_NAME "mouse-x11"

i32 mouse_X11_init(struct mouse_x11 *mouse, struct window_x11 *win)
{
    memset(mouse, 0, sizeof(struct mouse_x11));

    atomic_store(&mouse->x, 0);
    atomic_store(&mouse->y, 0);
    atomic_store(&mouse->button_bits, 0);

    /* Register mouse to enable X11 mouse event handling */
    if (window_X11_register_mouse(win, mouse)) return 1;

    mouse->win = win;

    return 0;
}

void mouse_X11_update(struct p_mouse *mouse)
{
    mouse->pos.x = atomic_load(&mouse->x11.x);
    mouse->pos.y = atomic_load(&mouse->x11.y);

    const u8 button_bits = atomic_load(&mouse->x11.button_bits);

    pressable_obj_update(&mouse->buttons[P_MOUSE_BUTTON_LEFT],
        button_bits & P_MOUSE_LEFTBUTTONMASK);
    pressable_obj_update(&mouse->buttons[P_MOUSE_BUTTON_RIGHT],
        button_bits & P_MOUSE_RIGHTBUTTONMASK);
    pressable_obj_update(&mouse->buttons[P_MOUSE_BUTTON_MIDDLE],
        button_bits & P_MOUSE_MIDDLEBUTTONMASK);
}

void mouse_X11_destroy(struct mouse_x11 *mouse)
{
    if (mouse == NULL) return;

    window_X11_deregister_mouse(mouse->win);
    memset(mouse, 0, sizeof(struct mouse_x11));
}
