#include "mouse.h"
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_mouse.h>

static void input_updateMouseButton(input_MouseButton* button, bool state)
{
    button->down = (state && !button->pressed);
    button->up = (!state && button->pressed);

    button->pressed = state;
    if(button->pressed)
        button->time++;
    else
        button->time = 0;
}

void input_updateMouse(input_Mouse* mouse)
{   
    Uint32 mouseState = SDL_GetMouseState(&mouse->x, &mouse->y);

    input_updateMouseButton(&mouse->buttonLeft, mouseState & SDL_BUTTON_LMASK);
    input_updateMouseButton(&mouse->buttonRight, mouseState & SDL_BUTTON_RMASK);
    input_updateMouseButton(&mouse->buttonMiddle, mouseState & SDL_BUTTON_MMASK);
}
