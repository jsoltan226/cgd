#ifndef P_MOUSE_H_
#define P_MOUSE_H_

#include "window.h"
#include <core/int.h>
#include <core/shapes.h>
#include <core/pressable-obj.h>

struct p_mouse;

enum p_mouse_button {
    P_MOUSE_BUTTON_LEFT,
    P_MOUSE_BUTTON_RIGHT,
    P_MOUSE_BUTTON_MIDDLE,
    P_MOUSE_N_BUTTONS
};

enum p_mouse_button_mask {
    P_MOUSE_LEFTBUTTONMASK   = 1 << P_MOUSE_BUTTON_LEFT,
    P_MOUSE_RIGHTBUTTONMASK  = 1 << P_MOUSE_BUTTON_RIGHT,
    P_MOUSE_MIDDLEBUTTONMASK = 1 << P_MOUSE_BUTTON_MIDDLE,
};

#define P_MOUSE_EVERYBUTTONMASK ((1 << P_MOUSE_N_BUTTONS) - 1)

struct p_mouse_state {
    pressable_obj_t buttons[P_MOUSE_N_BUTTONS];
    i32 x, y;
};

struct p_mouse * p_mouse_init(struct p_window *win, u32 flags);

void p_mouse_get_state(struct p_mouse *mouse,
    struct p_mouse_state *o, bool update);

#define p_mouse_update(mouse) p_mouse_get_state(mouse, NULL, true)

const pressable_obj_t * p_mouse_get_button(struct p_mouse *mouse,
    enum p_mouse_button button);

void p_mouse_force_release(struct p_mouse *mouse, u32 button_mask);

void p_mouse_destroy(struct p_mouse *mouse);

#endif /* P_MOUSE_H_ */
