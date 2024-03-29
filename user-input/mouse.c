#include "mouse.h"
#include <cgd/user-input/pressable-obj.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_mouse.h>
#include <assert.h>

ms_Mouse* ms_initMouse()
{
    /* Allocate the mouse struct on the heap */
    ms_Mouse *m = malloc(sizeof(ms_Mouse));
    assert(m != NULL);

    /* Initializethe mouse buttons */
    m->buttonLeft     = (po_PressableObj) { 0 };
    m->buttonRight    = (po_PressableObj) { 0 };
    m->buttonMiddle   = (po_PressableObj) { 0 };
    
    /* Set other members to default values */
    m->x = m->y = 0;

    return m;
}

void ms_updateMouse(ms_Mouse *mouse)
{   
    /* Get the mouse state from SDL, and update all the buttons accordingly */
    Uint32 mouseState = SDL_GetMouseState(&mouse->x, &mouse->y);

    po_updatePressableObj(&mouse->buttonLeft,   mouseState & SDL_BUTTON_LMASK);
    po_updatePressableObj(&mouse->buttonRight,  mouseState & SDL_BUTTON_RMASK);
    po_updatePressableObj(&mouse->buttonMiddle, mouseState & SDL_BUTTON_MMASK);
}

void ms_forceReleaseMouse(ms_Mouse *mouse, int buttons)
{
    /* It's pretty obvious what this does... */
    if(buttons & MS_LEFTBUTTONMASK){
        po_forceReleasePressableObj(&mouse->buttonLeft);
    }
    if(buttons & MS_RIGHTBUTTONMASK){
        po_forceReleasePressableObj(&mouse->buttonRight);
    }
    if(buttons & MS_MIDDLEBUTTONMASK){
        po_forceReleasePressableObj(&mouse->buttonMiddle);
    }
}

void ms_destroyMouse(ms_Mouse *mouse)
{
    free(mouse);
}
