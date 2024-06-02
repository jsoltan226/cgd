#ifndef MOUSE_H_
#define MOUSE_H_

#include <SDL2/SDL.h>
#include <stdbool.h>
#include "pressable-obj.h"
#include "core/int.h"

enum mouse_button {
    MOUSE_BUTTON_LEFT,
    MOUSE_BUTTON_RIGHT,
    MOUSE_BUTTON_MIDDLE,
    MOUSE_N_BUTTONS
};

enum mouse_button_mask {
    MOUSE_LEFTBUTTONMASK   = 1 << MOUSE_BUTTON_LEFT,
    MOUSE_RIGHTBUTTONMASK  = 1 << MOUSE_BUTTON_RIGHT,
    MOUSE_MIDDLEBUTTONMASK = 1 << MOUSE_BUTTON_MIDDLE,
};

#define MOUSE_EVERYBUTTONMASK ((1 << MOUSE_N_BUTTONS) - 1)

struct mouse {
    pressable_obj_t buttons[MOUSE_N_BUTTONS];
    i32 x, y;
};

struct mouse * mouse_init(void);
void mouse_update(struct mouse *mouse);
void mouse_force_release(struct mouse *mouse, u32 button_masks);
void mouse_destroy(struct mouse *mouse);

#endif /* MOUSE_H_ */
