#include "mouse.h"
#include "pressable-obj.h"
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_mouse.h>
#include <assert.h>

input_Mouse* input_initMouse()
{
    input_Mouse *m = malloc(sizeof(input_Mouse));
    assert(m != NULL);
    m->x = m->y = 0;
    m->buttonLeft = po_createPressableObj();
    m->buttonRight = po_createPressableObj();
    m->buttonMiddle = po_createPressableObj();
    
    return m;
}

void input_updateMouse(input_Mouse *mouse)
{   
    Uint32 mouseState = SDL_GetMouseState(&mouse->x, &mouse->y);

    po_updatePressableObj(&mouse->buttonLeft, mouseState & SDL_BUTTON_LMASK);
    po_updatePressableObj(&mouse->buttonRight, mouseState & SDL_BUTTON_RMASK);
    po_updatePressableObj(&mouse->buttonMiddle, mouseState & SDL_BUTTON_MMASK);
}

void input_forceReleaseMouse(input_Mouse *mouse, int buttons)
{
    if(buttons & INPUT_MOUSE_LEFTBUTTONMASK){
        po_forceReleasePressableObj(&mouse->buttonLeft);
    }
    if(buttons & INPUT_MOUSE_RIGHTBUTTONMASK){
        po_forceReleasePressableObj(&mouse->buttonRight);
    }
    if(buttons & INPUT_MOUSE_MIDDLEBUTTONMASK){
        po_forceReleasePressableObj(&mouse->buttonMiddle);
    }
}

void input_destroyMouse(input_Mouse *mouse)
{
    free(mouse);
}
