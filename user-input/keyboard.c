#include "keyboard.h"
#include <cgd/user-input/keyboard.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL_keycode.h>
#include <stdbool.h>
#include <assert.h>

kb_Keyboard* kb_initKeyboard()
{
    /* Allocate the keyboard array */
    kb_Keyboard *kb = malloc(sizeof(kb_Keyboard));
    assert(kb != NULL);

    /* Initialize all the keys */
    for(int i = 0; i < KB_KEYBOARD_LENGTH; i++)
    {
        (*kb)[i].key = po_createPressableObj();
        assert((*kb)[i].key != NULL);
        (*kb)[i].SDLKeycode = kb_correspondingSDLKeycodes[i];
    }
    return kb;
}

void kb_updateKeyboard(kb_Keyboard *kb)
{
    /* Get the keyboard state from SDL, and update all the keys accordingly */
    const Uint8* keystates = SDL_GetKeyboardState(NULL);
    
    for(int i = 0; i < KB_KEYBOARD_LENGTH; i++)
    {
        po_updatePressableObj((*kb)[i].key, keystates[(*kb)[i].SDLKeycode]);
    }
}

void kb_destroyKeyboard(kb_Keyboard *kb)
{
    /* Destroy all the individual keys, and then the keyboard array itself */
    for(int i = 0; i < KB_KEYBOARD_LENGTH; i++)
    {
        po_destroyPressableObj((*kb)[i].key);
    }
    free(kb);
    kb = NULL;
}

