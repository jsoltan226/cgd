#include "keyboard.h"
#include "core/pressable-obj.h"
#include "core/log.h"
#include "core/int.h"
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL_keycode.h>
#include <stdbool.h>

#define MODULE_NAME "keyboard"

static const SDL_Keycode kb_keycode_2_sdl_scancode_map[KB_KEYBOARD_LENGTH];

struct keyboard * keyboard_init(void)
{
    /* Allocate the keyboard array */
    struct keyboard *kb = malloc(sizeof(struct keyboard));
    s_assert(kb != NULL, "malloc() failed for struct keyboard");

    /* Initialize all the keys */
    for(u32 i = 0; i < KB_KEYBOARD_LENGTH; i++) {
        kb->keys[i].key = (pressable_obj_t) { 0 };
        kb->keys[i].SDLKeycode = kb_keycode_2_sdl_scancode_map[i];
    }
    s_log_debug("keyboard_init OK, %u unique keys", sizeof(kb->keys) / sizeof(*kb->keys));
    return kb;
}

void keyboard_update(struct keyboard *kb)
{
    if (kb == NULL) return;

    /* Get the keyboard state from SDL, and update all the keys accordingly */
    const u8* keystates = SDL_GetKeyboardState(NULL);

    for(u32 i = 0; i < KB_KEYBOARD_LENGTH; i++) {
        pressable_obj_update(&kb->keys[i].key, keystates[kb->keys[i].SDLKeycode]);
    }
}

void keyboard_destroy(struct keyboard *kb)
{
    if (kb == NULL) return;

    s_log_debug("Destroying keyboard...");
    free(kb);
}

static const SDL_Keycode kb_keycode_2_sdl_scancode_map[KB_KEYBOARD_LENGTH] = {
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
