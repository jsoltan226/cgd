#include "mouse.h"
#include "core/log.h"
#include "core/int.h"
#include "pressable-obj.h"
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_mouse.h>
#include <string.h>

struct mouse * mouse_init(void)
{
    struct mouse *m = calloc(1, sizeof(struct mouse));
    if (m == NULL) {
        s_log_error("mouse", "malloc() for struct mouse failed");
        return NULL;
    }

    m->x = 0;
    m->y = 0;

    s_log_debug("mouse", "mouse_init() OK!");
    return m;
}

void mouse_update(struct mouse *mouse)
{
    u32 state = SDL_GetMouseState(&mouse->x, &mouse->y);

    pressable_obj_update(&mouse->buttons[MOUSE_BUTTON_LEFT],   state & SDL_BUTTON_LMASK);
    pressable_obj_update(&mouse->buttons[MOUSE_BUTTON_RIGHT],  state & SDL_BUTTON_RMASK);
    pressable_obj_update(&mouse->buttons[MOUSE_BUTTON_MIDDLE], state & SDL_BUTTON_MMASK);
}

void mouse_force_release(struct mouse *mouse, u32 button_masks)
{
    s_log_debug("mouse", "Force releasing buttons: L: %i, R: %i, M: %i",
        0 < (button_masks & MOUSE_LEFTBUTTONMASK),
        0 < (button_masks & MOUSE_RIGHTBUTTONMASK),
        0 < (button_masks & MOUSE_MIDDLEBUTTONMASK)
    );
    for (u32 i = 0; i < MOUSE_N_BUTTONS; i++) {
        if (button_masks & (1 << i))
            pressable_obj_force_release(&mouse->buttons[i]);
    }
}

void mouse_destroy(struct mouse *mouse)
{
    s_log_debug("mouse", "Destroying mouse...");
    free(mouse);
}
