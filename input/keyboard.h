#ifndef KEYBOARD_H_
#define KEYBOARD_H_

#include "core/pressable-obj.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL_scancode.h>
#include <stdbool.h>

#define KB_KEYBOARD_LENGTH 41

struct keyboard_key {
    pressable_obj_t key;
    SDL_Keycode SDLKeycode;
};
struct keyboard {
    struct keyboard_key keys[KB_KEYBOARD_LENGTH];
};

struct keyboard * keyboard_init(void);
void keyboard_update(struct keyboard *kb);
void keyboard_destroy(struct keyboard *kb);

#endif /* KEYBOARD_H_ */
