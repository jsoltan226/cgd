#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL_scancode.h>
#include <stdbool.h>

#define INPUT_KEYBOARD_LENGTH               40

typedef struct {
    bool up;
    bool down;
    bool pressed;
    unsigned int time;
    SDL_Keycode SDLKeycode;
} input_Key;
typedef input_Key input_Keyboard[INPUT_KEYBOARD_LENGTH];

extern void input_initKeyboard(input_Keyboard keyboard);
extern void input_updateKeyboard(input_Keyboard keyboard);

typedef enum {
    INPUT_KEYCODE_SPACE,
    INPUT_KEYCODE_ESCAPE,
    INPUT_KEYCODE_DIGIT0,
    INPUT_KEYCODE_DIGIT1,
    INPUT_KEYCODE_DIGIT2,
    INPUT_KEYCODE_DIGIT3,
    INPUT_KEYCODE_DIGIT4,
    INPUT_KEYCODE_DIGIT5,
    INPUT_KEYCODE_DIGIT6,
    INPUT_KEYCODE_DIGIT7,
    INPUT_KEYCODE_DIGIT8,
    INPUT_KEYCODE_DIGIT9,
    INPUT_KEYCODE_A,
    INPUT_KEYCODE_B,
    INPUT_KEYCODE_C,
    INPUT_KEYCODE_D,
    INPUT_KEYCODE_E,
    INPUT_KEYCODE_F,
    INPUT_KEYCODE_G,
    INPUT_KEYCODE_H,
    INPUT_KEYCODE_I,
    INPUT_KEYCODE_J,
    INPUT_KEYCODE_K,
    INPUT_KEYCODE_L,
    INPUT_KEYCODE_M,
    INPUT_KEYCODE_N,
    INPUT_KEYCODE_O,
    INPUT_KEYCODE_P,
    INPUT_KEYCODE_Q,
    INPUT_KEYCODE_S,
    INPUT_KEYCODE_T,
    INPUT_KEYCODE_U,
    INPUT_KEYCODE_V,
    INPUT_KEYCODE_W,
    INPUT_KEYCODE_X,
    INPUT_KEYCODE_Y,
    INPUT_KEYCODE_Z,
} input_KeyCode;

static const SDL_Keycode input_correspondingSDLKeycodes[INPUT_KEYBOARD_LENGTH] = {
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
};

#endif
