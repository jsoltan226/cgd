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
    [KB_KEYCODE_A]          = 'A',
    [KB_KEYCODE_B]          = 'B',
    [KB_KEYCODE_C]          = 'C',
    [KB_KEYCODE_D]          = 'D',
    [KB_KEYCODE_E]          = 'E',
    [KB_KEYCODE_F]          = 'F',
    [KB_KEYCODE_G]          = 'G',
    [KB_KEYCODE_H]          = 'H',
    [KB_KEYCODE_I]          = 'I',
    [KB_KEYCODE_J]          = 'J',
    [KB_KEYCODE_K]          = 'K',
    [KB_KEYCODE_L]          = 'L',
    [KB_KEYCODE_M]          = 'M',
    [KB_KEYCODE_N]          = 'N',
    [KB_KEYCODE_O]          = 'O',
    [KB_KEYCODE_P]          = 'P',
    [KB_KEYCODE_Q]          = 'Q',
    [KB_KEYCODE_R]          = 'R',
    [KB_KEYCODE_S]          = 'S',
    [KB_KEYCODE_T]          = 'T',
    [KB_KEYCODE_U]          = 'U',
    [KB_KEYCODE_V]          = 'V',
    [KB_KEYCODE_W]          = 'W',
    [KB_KEYCODE_X]          = 'X',
    [KB_KEYCODE_Y]          = 'Y',
    [KB_KEYCODE_Z]          = 'Z',
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
    u_check_params(kb != NULL);

    for (u32 i = 0; i < P_KEYBOARD_N_KEYS; i++) {
        if (GetAsyncKeyState(keycode_map[i]) & 0x8000) {
            pressable_obj_update(&kb->pobjs[i], true);
        } else if (kb->pobjs[i].pressed || kb->pobjs[i].up) {
            pressable_obj_update(&kb->pobjs[i], false);
        }
    }
}

const pressable_obj_t * p_keyboard_get_key(const struct p_keyboard *kb,
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
