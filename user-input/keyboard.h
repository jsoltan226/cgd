#pragma once
#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "pressable-obj.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL_scancode.h>
#include <stdbool.h>

#define KB_KEYBOARD_LENGTH                41

typedef struct {
    po_PressableObj *key;
    SDL_Keycode SDLKeycode;
} kb_Key;
typedef kb_Key kb_Keyboard[KB_KEYBOARD_LENGTH];

kb_Keyboard* kb_initKeyboard();
void kb_updateKeyboard(kb_Keyboard *kb);
void kb_destroyKeyboard(kb_Keyboard *kb);

#define kb_getKey(kbPtr, kc)                (*kbPtr)[kc].key

typedef enum {
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
} kb_KeyCode;

static const SDL_Keycode kb_correspondingSDLKeycodes[KB_KEYBOARD_LENGTH] = {
    SDL_SCANCODE_SPACE,
    SDL_SCANCODE_ESCAPE,
    SDL_SCANCODE_0,
    SDL_SCANCODE_1,
    SDL_SCANCODE_2,
    SDL_SCANCODE_3,
    SDL_SCANCODE_4,
    SDL_SCANCODE_5,
    SDL_SCANCODE_6,
    SDL_SCANCODE_7,
    SDL_SCANCODE_8,
    SDL_SCANCODE_9,
    SDL_SCANCODE_A,
    SDL_SCANCODE_B,
    SDL_SCANCODE_C,
    SDL_SCANCODE_D,
    SDL_SCANCODE_E,
    SDL_SCANCODE_F,
    SDL_SCANCODE_G,
    SDL_SCANCODE_H,
    SDL_SCANCODE_I,
    SDL_SCANCODE_J,
    SDL_SCANCODE_K,
    SDL_SCANCODE_L,
    SDL_SCANCODE_M,
    SDL_SCANCODE_N,
    SDL_SCANCODE_O,
    SDL_SCANCODE_P,
    SDL_SCANCODE_Q,
    SDL_SCANCODE_S,
    SDL_SCANCODE_T,
    SDL_SCANCODE_U,
    SDL_SCANCODE_V,
    SDL_SCANCODE_W,
    SDL_SCANCODE_X,
    SDL_SCANCODE_Y,
    SDL_SCANCODE_Z,
    SDL_SCANCODE_UP,
    SDL_SCANCODE_DOWN,
    SDL_SCANCODE_LEFT,
    SDL_SCANCODE_RIGHT,
};

#endif
