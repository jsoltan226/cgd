#include "../mouse.h"
#include "core/int.h"
#include "core/log.h"
#include "core/pressable-obj.h"
#include "core/util.h"
#include <stdbool.h>
#include <stdlib.h>
#define P_INTERNAL_GUARD__
#include "mouse-x11.h"
#undef P_INTERNAL_GUARD__
#define P_INTERNAL_GUARD__
#include "mouse-devinput.h"
#undef P_INTERNAL_GUARD__
#define P_INTERNAL_GUARD__
#include "window-internal.h"
#undef P_INTERNAL_GUARD__

#define MODULE_NAME "mouse"

#define N_MOUSE_TYPES 3
#define MOUSE_TYPES_LIST    \
    X_(MOUSE_TYPE_FAIL)     \
    X_(MOUSE_TYPE_X11)      \
    X_(MOUSE_TYPE_DEVINPUT) \

#define X_(name) name,
enum mouse_type {
    MOUSE_TYPES_LIST
};
#undef X_

#define X_(name) #name,
static const char * const mouse_type_strings[N_MOUSE_TYPES] = {
    MOUSE_TYPES_LIST
};
#undef X_

static const enum mouse_type
    mouse_fallback_modes[N_WINDOW_TYPES][N_MOUSE_TYPES] = {
    [WINDOW_TYPE_X11] = {
        MOUSE_TYPE_DEVINPUT,
        MOUSE_TYPE_X11,
        MOUSE_TYPE_FAIL
    },
    [WINDOW_TYPE_FRAMEBUFFER] = {
        MOUSE_TYPE_DEVINPUT,
        MOUSE_TYPE_FAIL
    },
    [WINDOW_TYPE_DUMMY] = {
        MOUSE_TYPE_DEVINPUT,
        MOUSE_TYPE_X11,
        MOUSE_TYPE_FAIL
    }
};

struct p_mouse {
    enum mouse_type type;
    union {
        struct mouse_x11 x11;
        struct mouse_devinput devinput;
    };

    pressable_obj_t buttons[P_MOUSE_N_BUTTONS];
};

struct p_mouse * p_mouse_init(struct p_window *win, u32 flags)
{
    u_check_params(win != NULL);

    struct p_mouse *m = calloc(1, sizeof(struct p_mouse));
    s_assert(m != NULL, "calloc() failed for struct mouse");

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
            case MOUSE_TYPE_DEVINPUT:
                if (mouse_devinput_init(&m->devinput, flags))
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
    s_log_debug("%s() OK, mouse is type \"%s\"", __func__, m->type);
    
    return m;

err:
    p_mouse_destroy(m);
    return NULL;
}

void p_mouse_get_state(struct p_mouse *mouse,
    struct p_mouse_state *o, bool update)
{
    u_check_params(mouse != NULL);
    switch (mouse->type) {
        case MOUSE_TYPE_X11:
            mouse_X11_get_state(&mouse->x11, o, update);
            break;
        case MOUSE_TYPE_DEVINPUT:
            mouse_devinput_get_state(&mouse->devinput, o, update);
            break;
        default:
            break;
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

void p_mouse_destroy(struct p_mouse *mouse)
{
    if (mouse == NULL) return;

    s_log_debug("Destroying mouse (type \"%s\")",
        mouse_type_strings[mouse->type]);

    switch(mouse->type) {
        case MOUSE_TYPE_X11:
            mouse_X11_destroy(&mouse->x11);
            break;
        case MOUSE_TYPE_DEVINPUT:
            mouse_devinput_destroy(&mouse->devinput);
            break;
        default: case MOUSE_TYPE_FAIL:
            break;
    }

    free(mouse);
}
