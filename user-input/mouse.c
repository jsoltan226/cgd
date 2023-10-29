#include "mouse.h"
#include "pressable-obj.h"
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_mouse.h>
#include <assert.h>

ms_Mouse* ms_initMouse()
{
    ms_Mouse *m = malloc(sizeof(ms_Mouse));
    assert(m != NULL);
    m->x = m->y = 0;
    m->buttonLeft = po_createPressableObj();
    m->buttonRight = po_createPressableObj();
    m->buttonMiddle = po_createPressableObj();
    
    return m;
}

void ms_updateMouse(ms_Mouse *mouse)
{   
    Uint32 mouseState = SDL_GetMouseState(&mouse->x, &mouse->y);

    po_updatePressableObj(mouse->buttonLeft, mouseState & SDL_BUTTON_LMASK);
    po_updatePressableObj(mouse->buttonRight, mouseState & SDL_BUTTON_RMASK);
    po_updatePressableObj(mouse->buttonMiddle, mouseState & SDL_BUTTON_MMASK);
}

void ms_forceReleaseMouse(ms_Mouse *mouse, int buttons)
{
    if(buttons & MS_LEFTBUTTONMASK){
        po_forceReleasePressableObj(mouse->buttonLeft);
    }
    if(buttons & MS_RIGHTBUTTONMASK){
        po_forceReleasePressableObj(mouse->buttonRight);
    }
    if(buttons & MS_MIDDLEBUTTONMASK){
        po_forceReleasePressableObj(mouse->buttonMiddle);
    }
}

void ms_destroyMouse(ms_Mouse *mouse)
{
    po_destroyPressableObj(mouse->buttonLeft);
    po_destroyPressableObj(mouse->buttonRight);
    po_destroyPressableObj(mouse->buttonMiddle);
    free(mouse);
    mouse = NULL;
}
