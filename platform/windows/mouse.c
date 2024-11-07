#include "../mouse.h"
#include "../window.h"
#include <core/log.h>
#include <core/int.h>
#include <core/util.h>
#include <core/math.h>
#include <core/shapes.h>
#include <core/pressable-obj.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif /* WIN32_LEAN_AND_MEAN */
#include <windows.h>
#include <errhandlingapi.h>
#define P_INTERNAL_GUARD__
#include "window-internal.h"
#undef P_INTERNAL_GUARD__
#define P_INTERNAL_GUARD__
#include "error.h"
#undef P_INTERNAL_GUARD__

#define MODULE_NAME "mouse"

struct p_mouse {
    pressable_obj_t buttons[P_MOUSE_N_BUTTONS];
    vec2d_t pos;
    bool is_out_of_window;
    struct p_window *win;
};

struct p_mouse * p_mouse_init(struct p_window *win, u32 flags)
{
    u_check_params(win != NULL);
    (void) flags;

    struct p_mouse *m = calloc(1, sizeof(struct p_mouse));
    s_assert(m != NULL, "calloc() failed for struct mouse");

    m->win = win;

    return m;
}

void p_mouse_get_state(struct p_mouse *mouse,
    struct p_mouse_state *o, bool update)
{
    if (update) {
        pressable_obj_update(&mouse->buttons[P_MOUSE_BUTTON_LEFT],
            GetAsyncKeyState(VK_LBUTTON) & 0x8000);
        pressable_obj_update(&mouse->buttons[P_MOUSE_BUTTON_RIGHT],
            GetAsyncKeyState(VK_RBUTTON) & 0x8000);
        pressable_obj_update(&mouse->buttons[P_MOUSE_BUTTON_MIDDLE],
            GetAsyncKeyState(VK_MBUTTON) & 0x8000);

        POINT screen_pos, window_pos;
        if (GetCursorPos(&screen_pos) == 0) {
            s_log_error("Failed to get mouse cursor position: %s",
                get_last_error_msg());
            goto update_end;
        }
        memcpy(&window_pos, &screen_pos, sizeof(POINT));

        if (ScreenToClient(mouse->win->win, &window_pos) == 0) {
            s_log_error("Failed to translate mouse screen coordinates"
                "to window coordinates: %s", get_last_error_msg());
            goto update_end;
        }

        mouse->pos.x = (f32)window_pos.x;
        mouse->pos.y = (f32)window_pos.y;

        mouse->is_out_of_window = !u_collision(
            &mouse->win->client_rect,
            &(const rect_t) { mouse->pos.x, mouse->pos.y, 0, 0 }
        );
    }
update_end:

    if (o != NULL) {
        memcpy(o->buttons, mouse->buttons,
                sizeof(pressable_obj_t) * P_MOUSE_N_BUTTONS);

        o->x = mouse->pos.x;
        o->y = mouse->pos.y;
        o->is_out_of_window = mouse->is_out_of_window;
    }
}

const pressable_obj_t * p_mouse_get_button(struct p_mouse *mouse,
    enum p_mouse_button button)
{
    u_check_params(mouse != NULL && button >= 0 && button < P_MOUSE_N_BUTTONS);
    return &mouse->buttons[button];
}

void p_mouse_reset(struct p_mouse *mouse, u32 button_mask)
{
    u_check_params(mouse != NULL);

    if (button_mask & P_MOUSE_LEFTBUTTONMASK)
        pressable_obj_reset(&mouse->buttons[P_MOUSE_BUTTON_LEFT]);
    if (button_mask & P_MOUSE_RIGHTBUTTONMASK)
        pressable_obj_reset(&mouse->buttons[P_MOUSE_BUTTON_RIGHT]);
    if (button_mask & P_MOUSE_MIDDLEBUTTONMASK)
        pressable_obj_reset(&mouse->buttons[P_MOUSE_BUTTON_MIDDLE]);
}

void p_mouse_force_release(struct p_mouse *mouse, u32 button_mask)
{
    u_check_params(mouse != NULL);

    if (button_mask & P_MOUSE_LEFTBUTTONMASK)
        pressable_obj_force_release(&mouse->buttons[P_MOUSE_BUTTON_LEFT]);
    if (button_mask & P_MOUSE_RIGHTBUTTONMASK)
        pressable_obj_force_release(&mouse->buttons[P_MOUSE_BUTTON_RIGHT]);
    if (button_mask & P_MOUSE_MIDDLEBUTTONMASK)
        pressable_obj_force_release(&mouse->buttons[P_MOUSE_BUTTON_MIDDLE]);
}

void p_mouse_destroy(struct p_mouse **mouse_p)
{
    if (mouse_p == NULL || *mouse_p == NULL)
        return;

    u_nzfree(mouse_p);
}
