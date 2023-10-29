#include "keyboard.h"
#include "pressable-obj.h"
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL_keycode.h>
#include <stdbool.h>

kb_Keyboard* kb_initKeyboard()
{
    kb_Keyboard *kb = malloc(sizeof(kb_Keyboard));
    for(int i = 0; i < KB_KEYBOARD_LENGTH; i++)
    {
        (*kb)[i].key = po_createPressableObj();
        (*kb)[i].SDLKeycode = kb_correspondingSDLKeycodes[i];
    }
    return kb;
}

void kb_updateKeyboard(kb_Keyboard *kb)
{
    const Uint8* keystates = SDL_GetKeyboardState(NULL);
    
    for(int i = 0; i < KB_KEYBOARD_LENGTH; i++)
    {
        po_updatePressableObj((*kb)[i].key, keystates[(*kb)[i].SDLKeycode]);
    }
}

void kb_destroyKeyboard(kb_Keyboard *kb)
{
    for(int i = 0; i < KB_KEYBOARD_LENGTH; i++)
    {
        po_destroyPressableObj((*kb)[i].key);
    }
    free(kb);
    kb = NULL;
}

