#include "../mouse.h"
#include <core/math.h>
#include <core/int.h>
#include <core/log.h>
#include <core/util.h>
#include <core/pressable-obj.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#define P_INTERNAL_GUARD__
#include "mouse-x11.h"
#undef P_INTERNAL_GUARD__
#define P_INTERNAL_GUARD__
#include "mouse-evdev.h"
#undef P_INTERNAL_GUARD__
#define P_INTERNAL_GUARD__
#include "window-internal.h"
#undef P_INTERNAL_GUARD__
#define P_INTERNAL_GUARD__
#include "mouse-internal.h"
#undef P_INTERNAL_GUARD__

#define MODULE_NAME "mouse"

struct p_mouse * p_mouse_init(struct p_window *win, u32 flags)
{
    u_check_params(win != NULL);

    struct p_mouse *m = calloc(1, sizeof(struct p_mouse));
    s_assert(m != NULL, "calloc() failed for struct mouse");

    m->win = win;

    u32 i = 0;
    do {
        m->type = mouse_fallback_modes[win->type][i];
        switch(m->type) {
            case MOUSE_TYPE_X11:
                if (mouse_X11_init(&m->x11, &win->x11, flags))
                    s_log_warn("Failed to set up mouse with X11");
                else
                    goto mouse_setup_success;
                break;
            case MOUSE_TYPE_EVDEV:
                /* Set the mouse X and Y to the middle of the window */
                m->pos.x = (f32)(win->x + win->w) / 2.f;
                m->pos.y = (f32)(win->y + win->h) / 2.f;
                if (mouse_evdev_init(&m->evdev, flags))
                    s_log_warn("Failed to set up mouse using /dev/input");
                else
                    goto mouse_setup_success;
                break;
            default: case MOUSE_TYPE_FAIL:
                goto_error("Failed to set up mouse in any known way");
                break;
        }
    } while (mouse_fallback_modes[win->type][i++] != MOUSE_TYPE_FAIL);

mouse_setup_success:
    s_log_info("%s() OK, mouse is type \"%s\"",
        __func__, mouse_type_strings[m->type]);
    
    return m;

err:
    p_mouse_destroy(&m);
    return NULL;
}

void p_mouse_get_state(struct p_mouse *mouse,
    struct p_mouse_state *o, bool update)
{
    u_check_params(mouse != NULL);

    if (update) {
        switch (mouse->type) {
            case MOUSE_TYPE_X11:
                mouse_X11_update(mouse);
                break;
            case MOUSE_TYPE_EVDEV:
                mouse_evdev_update(mouse);
                break;
            default:
                break;
        }
        
        const rect_t
        mouse_r = {
            .x = mouse->pos.x + mouse->win->ev_offset.x,
            .y = mouse->pos.y + mouse->win->ev_offset.y,
            .w = 0,
            .h = 0,
        },
        window_r = {
            .x = 0,
            .y = 0,
            .w = mouse->win->w,
            .h = mouse->win->h
        };
        mouse->is_out_of_window = !u_collision(&mouse_r, &window_r);

        /* Discard the button presses if they occur outside of the window */
        for (u32 i = 0; i < P_MOUSE_N_BUTTONS; i++) {
            if (mouse->buttons[i].up && mouse->is_out_of_window)
                pressable_obj_reset(&mouse->buttons[i]);
        }
    }

    if (o != NULL) {
        memcpy(o->buttons, mouse->buttons, sizeof(mouse->buttons));
        o->x = mouse->pos.x;
        o->y = mouse->pos.y;
    }
}

const pressable_obj_t * p_mouse_get_button(struct p_mouse *mouse,
    enum p_mouse_button button)
{
    u_check_params(mouse != NULL);
    return &mouse->buttons[button];
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
    if (mouse_p == NULL || *mouse_p == NULL) return;

    struct p_mouse *mouse = *mouse_p;

    s_log_debug("Destroying mouse (type \"%s\")",
        mouse_type_strings[mouse->type]);

    switch(mouse->type) {
        case MOUSE_TYPE_X11:
            mouse_X11_destroy(&mouse->x11);
            break;
        case MOUSE_TYPE_EVDEV:
            mouse_evdev_destroy(&mouse->evdev);
            break;
        default: case MOUSE_TYPE_FAIL:
            break;
    }

    u_nzfree(mouse);
}
