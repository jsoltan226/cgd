#include "keyboard.h"
#include "pressable-obj.h"
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_keyboard.h>
#include <stdbool.h>

void input_initKeyboard(input_Keyboard keyboard)
{
    for(int i = 0; i < INPUT_KEYBOARD_LENGTH; i++)
    {
        keyboard[i].key = po_createPressableObj();
        keyboard[i].SDLKeycode = input_correspondingSDLKeycodes[i];
    }
}

void input_updateKeyboard(input_Keyboard keyboard)
{
    const Uint8* keystates = SDL_GetKeyboardState(NULL);
    
    for(int i = 0; i < INPUT_KEYBOARD_LENGTH; i++)
    {
        po_updatePressableObj(&keyboard[i].key, keystates[keyboard[i].SDLKeycode]);
    }
}
