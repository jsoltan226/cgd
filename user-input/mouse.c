#include "mouse.h"
#include "pressable-obj.h"
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_mouse.h>

void input_updateMouse(input_Mouse* mouse)
{   
    Uint32 mouseState = SDL_GetMouseState(&mouse->x, &mouse->y);

    po_updatePressableObj(&mouse->buttonLeft, mouseState & SDL_BUTTON_LMASK);
    po_updatePressableObj(&mouse->buttonRight, mouseState & SDL_BUTTON_RMASK);
    po_updatePressableObj(&mouse->buttonMiddle, mouseState & SDL_BUTTON_MMASK);
}
