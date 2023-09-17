#include "keyboard.h"
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_keyboard.h>
#include <stdbool.h>

void input_initKeyboard(input_Keyboard keyboard)
{
    for(int i = 0; i < INPUT_KEYBOARD_LENGTH; i++)
    {
        keyboard[i].up = false;
        keyboard[i].down = false;
        keyboard[i].pressed = false;
        keyboard[i].time = 0;
        keyboard[i].SDLKeycode = input_correspondingSDLKeycodes[i];
    }
}

void input_updateKeyboard(input_Keyboard keyboard)
{
    const Uint8* keystates = SDL_GetKeyboardState(NULL);
    
    for(int i = 0; i < INPUT_KEYBOARD_LENGTH; i++)
    {
        bool keystate = keystates[keyboard[i].SDLKeycode];

        keyboard[i].down = (keystate && !keyboard[i].pressed);
        keyboard[i].up = (!keystate && keyboard[i].pressed);
         
        keyboard[i].pressed = keystate;
        if(keyboard[i].pressed)
            keyboard[i].time++;
        else
            keyboard[i].time = 0;
    }
}
