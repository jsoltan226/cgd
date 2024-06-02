#ifndef KEYBOARD_H_
#define KEYBOARD_H_

#include "pressable-obj.h"
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

#define kb_getKey(kbPtr, keycode) kbPtr->keys[keycode].key

enum keyboard_keycode {
    KB_KEYCODE_SPACE,
    KB_KEYCODE_ESCAPE,
    KB_KEYCODE_DIGIT0,
    KB_KEYCODE_DIGIT1,
    KB_KEYCODE_DIGIT2,
    KB_KEYCODE_DIGIT3,
    KB_KEYCODE_DIGIT4,
    KB_KEYCODE_DIGIT5,
    KB_KEYCODE_DIGIT6,
    KB_KEYCODE_DIGIT7,
    KB_KEYCODE_DIGIT8,
    KB_KEYCODE_DIGIT9,
    KB_KEYCODE_A,
    KB_KEYCODE_B,
    KB_KEYCODE_C,
    KB_KEYCODE_D,
    KB_KEYCODE_E,
    KB_KEYCODE_F,
    KB_KEYCODE_G,
    KB_KEYCODE_H,
    KB_KEYCODE_I,
    KB_KEYCODE_J,
    KB_KEYCODE_K,
    KB_KEYCODE_L,
    KB_KEYCODE_M,
    KB_KEYCODE_N,
    KB_KEYCODE_O,
    KB_KEYCODE_P,
    KB_KEYCODE_Q,
    KB_KEYCODE_S,
    KB_KEYCODE_T,
    KB_KEYCODE_U,
    KB_KEYCODE_V,
    KB_KEYCODE_W,
    KB_KEYCODE_X,
    KB_KEYCODE_Y,
    KB_KEYCODE_Z,
    KB_KEYCODE_ARROWUP,
    KB_KEYCODE_ARROWDOWN,
    KB_KEYCODE_ARROWLEFT,
    KB_KEYCODE_ARROWRIGHT,
};

#endif /* KEYBOARD_H_ */
