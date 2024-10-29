#include "../keyboard.h"
#include <core/int.h>
#include <core/log.h>
#include <core/util.h>
#include <core/pressable-obj.h>
#include <stdlib.h>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif /* WIN32_LEAN_AND_MEAN */
#include <windows.h>

#define MODULE_NAME "keyboard"

#define N_VIRTUAL_KEYS 256
struct p_keyboard {
    pressable_obj_t pobjs[P_KEYBOARD_N_KEYS];
};

static const i32 keycode_map[P_KEYBOARD_N_KEYS] = {
    [KB_KEYCODE_ENTER]      = VK_RETURN,
    [KB_KEYCODE_SPACE]      = VK_SPACE,
    [KB_KEYCODE_ESCAPE]     = VK_ESCAPE,
    [KB_KEYCODE_DIGIT0]     = '0',
    [KB_KEYCODE_DIGIT1]     = '1',
    [KB_KEYCODE_DIGIT2]     = '2',
    [KB_KEYCODE_DIGIT3]     = '3',
    [KB_KEYCODE_DIGIT4]     = '4',
    [KB_KEYCODE_DIGIT5]     = '5',
    [KB_KEYCODE_DIGIT6]     = '6',
    [KB_KEYCODE_DIGIT7]     = '7',
    [KB_KEYCODE_DIGIT8]     = '8',
    [KB_KEYCODE_DIGIT9]     = '9',
    [KB_KEYCODE_A]          = 'a',
    [KB_KEYCODE_B]          = 'b',
    [KB_KEYCODE_C]          = 'c',
    [KB_KEYCODE_D]          = 'd',
    [KB_KEYCODE_E]          = 'e',
    [KB_KEYCODE_F]          = 'f',
    [KB_KEYCODE_G]          = 'g',
    [KB_KEYCODE_H]          = 'h',
    [KB_KEYCODE_I]          = 'i',
    [KB_KEYCODE_J]          = 'j',
    [KB_KEYCODE_K]          = 'k',
    [KB_KEYCODE_L]          = 'l',
    [KB_KEYCODE_M]          = 'm',
    [KB_KEYCODE_N]          = 'n',
    [KB_KEYCODE_O]          = 'o',
    [KB_KEYCODE_P]          = 'p',
    [KB_KEYCODE_Q]          = 'q',
    [KB_KEYCODE_R]          = 'r',
    [KB_KEYCODE_S]          = 's',
    [KB_KEYCODE_T]          = 't',
    [KB_KEYCODE_U]          = 'u',
    [KB_KEYCODE_V]          = 'v',
    [KB_KEYCODE_W]          = 'w',
    [KB_KEYCODE_X]          = 'x',
    [KB_KEYCODE_Y]          = 'y',
    [KB_KEYCODE_Z]          = 'z',
    [KB_KEYCODE_ARROWUP]    = VK_UP,
    [KB_KEYCODE_ARROWDOWN]  = VK_DOWN,
    [KB_KEYCODE_ARROWLEFT]  = VK_LEFT,
    [KB_KEYCODE_ARROWRIGHT] = VK_RIGHT,
};

struct p_keyboard * p_keyboard_init(struct p_window *win)
{
    u_check_params(win != NULL);
    (void) win;

    struct p_keyboard *kb = calloc(1, sizeof(struct p_keyboard));
    s_assert(kb != NULL, "calloc() failed for struct keyboard");

    return kb;
}

void p_keyboard_update(struct p_keyboard *kb)
{
    for (u32 i = 0; i < P_KEYBOARD_N_KEYS; i++) {
        if (GetAsyncKeyState(keycode_map[i]) & 0x8000) {
            pressable_obj_update(&kb->pobjs[i], true);
        } else if (kb->pobjs[i].pressed || kb->pobjs[i].down) {
            pressable_obj_update(&kb->pobjs[i], false);
        }
    }
}

const pressable_obj_t * p_keyboard_get_key(struct p_keyboard *kb,
    enum p_keyboard_keycode code)
{
    u_check_params(kb != NULL && code >= 0 && code < P_KEYBOARD_N_KEYS);
    return &kb->pobjs[code];
}

void p_keyboard_destroy(struct p_keyboard **kb_p)
{
    if (kb_p == NULL || *kb_p == NULL)
        return;

    u_nzfree(kb_p);
}
