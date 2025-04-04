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
    bool is_out_of_window;
};

struct p_mouse * p_mouse_init(struct p_window *win);

void p_mouse_update(struct p_mouse *mouse);

void p_mouse_get_state(const struct p_mouse *mouse, struct p_mouse_state *o);

const pressable_obj_t * p_mouse_get_button(const struct p_mouse *mouse,
    enum p_mouse_button button);

void p_mouse_reset(struct p_mouse *mouse, u32 button_mask);
void p_mouse_force_release(struct p_mouse *mouse, u32 button_mask);

void p_mouse_destroy(struct p_mouse **mouse_p);

#endif /* P_MOUSE_H_ */
