#include "mouse.h"
#include "pressable-obj.h"
#include <cgd/util/int.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_mouse.h>
#include <assert.h>
#include <string.h>

ms_Mouse* ms_initMouse()
{
    ms_Mouse *m = calloc(1, sizeof(ms_Mouse));
    assert(m != NULL);

    m->x = m->y = 0;

    return m;
}

void ms_updateMouse(ms_Mouse *mouse)
{   
    u32 mouseState = SDL_GetMouseState(&mouse->x, &mouse->y);

    po_updatePressableObj(&mouse->buttons[MS_BUTTON_LEFT],   mouseState & SDL_BUTTON_LMASK);
    po_updatePressableObj(&mouse->buttons[MS_BUTTON_RIGHT],  mouseState & SDL_BUTTON_RMASK);
    po_updatePressableObj(&mouse->buttons[MS_BUTTON_MIDDLE], mouseState & SDL_BUTTON_MMASK);
}

void ms_forceReleaseMouse(ms_Mouse *mouse, u32 button_masks)
{
    for (u32 i = 0; i < MS_N_BUTTONS; i++) {
        if (button_masks & (1 << i))
            po_forceReleasePressableObj(&mouse->buttons[i]);
    }
}

void ms_destroyMouse(ms_Mouse *mouse)
{
    free(mouse);
}
