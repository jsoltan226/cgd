#include "../keyboard.h"
#include "core/int.h"
#include "core/log.h"
#include "core/pressable-obj.h"
#include "core/util.h"
#include <stdbool.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#define P_INTERNAL_GUARD__
#include "window-internal.h"
#undef P_INTERNAL_GUARD__
#define P_INTERNAL_GUARD__
#include "keyboard-tty.h"
#undef P_INTERNAL_GUARD__
#define P_INTERNAL_GUARD__
#include "keyboard-evdev.h"
#undef P_INTERNAL_GUARD__
#define P_INTERNAL_GUARD__
#include "keyboard-x11.h"
#undef P_INTERNAL_GUARD__
#define P_INTERNAL_GUARD__
#include "window-x11.h"
#undef P_INTERNAL_GUARD__

#define MODULE_NAME "keyboard"

#define N_KEYBOARD_MODES 4
#define KB_MODES_LIST   \
    X_(KB_MODE_FAIL)    \
    X_(KB_MODE_EVDEV)   \
    X_(KB_MODE_TTY)     \
    X_(KB_MODE_X11)     \

#define X_(name) name,
enum keyboard_mode {
    KB_MODES_LIST
};
#undef X_

#define X_(name) #name,
static const char *keyboard_mode_strings[N_KEYBOARD_MODES] = {
    KB_MODES_LIST
};
#undef X_

static const enum keyboard_mode fallback_modes[N_WINDOW_TYPES][N_KEYBOARD_MODES] = {
    [WINDOW_TYPE_FRAMEBUFFER] = {
        KB_MODE_EVDEV,
        KB_MODE_TTY,
        KB_MODE_FAIL
    },
    [WINDOW_TYPE_X11] = {
        KB_MODE_X11,
        KB_MODE_FAIL
    },
    [WINDOW_TYPE_DUMMY] = {
        KB_MODE_EVDEV,
        KB_MODE_TTY,
        KB_MODE_FAIL
    },
};

struct p_keyboard {
    enum keyboard_mode mode;
    
    union {
        struct keyboard_evdev evdev;
        struct keyboard_tty tty;
        struct keyboard_x11 x11;
    };

    pressable_obj_t keys[P_KEYBOARD_N_KEYS];
};

struct p_keyboard * p_keyboard_init(struct p_window *win)
{
    struct p_keyboard *kb = calloc(1, sizeof(struct p_keyboard));
    s_assert(kb != NULL, "calloc() failed for struct p_keyboard");

    enum window_type win_type;
#define DEFAULT_WINDOW_FALLBACK_MODE WINDOW_TYPE_FRAMEBUFFER
    if (win == NULL) {
        s_log_warn("Cannot get window metadata. Assuming window type is %s",
            window_type_strings[DEFAULT_WINDOW_FALLBACK_MODE]);
        win_type = DEFAULT_WINDOW_FALLBACK_MODE;
    } else {
        win_type = win->type;
    }

    u32 i = 0;
    do {
        s_log_debug("Attempting keyboard init with mode \"%s\"...",
            keyboard_mode_strings[ fallback_modes[win_type][i] ]
        );
        switch (fallback_modes[win_type][i]) {
            case KB_MODE_EVDEV:
                if (evdev_keyboard_init(&kb->evdev))
                    s_log_warn("Failed to set up keyboard using /dev/input");
                else
                    goto keyboard_setup_success;
                break;
            case KB_MODE_TTY:
                if (tty_keyboard_init(&kb->tty))
                    s_log_warn("Failed to set up keyboard using tty");
                else
                    goto keyboard_setup_success;
                break;
            case KB_MODE_X11:
                if (X11_keyboard_init(&kb->x11, &win->x11))
                    s_log_warn("Failed to set up keyboard with X11");
                else
                    goto keyboard_setup_success;
                break;
            default: case KB_MODE_FAIL:
                goto_error("Failed to set up keyboard in any known way");
                break;
        };
    } while (++i < N_KEYBOARD_MODES);

keyboard_setup_success:
    kb->mode = fallback_modes[win_type][i];
    s_log_info("%s() OK, keyboard is in mode \"%s\"", __func__, keyboard_mode_strings[kb->mode]);

    return kb;

err:
    p_keyboard_destroy(kb);
    return NULL;
}

void p_keyboard_update(struct p_keyboard *kb)
{
    if (kb == NULL) return;

    switch (kb->mode) {
        case KB_MODE_TTY:
            tty_keyboard_update_all_keys(&kb->tty, kb->keys);
            break;
        case KB_MODE_EVDEV:
            evdev_keyboard_update_all_keys(&kb->evdev, kb->keys);
            break;
        case KB_MODE_X11:
            X11_keyboard_update_all_keys(&kb->x11, kb->keys);
            break;
        default:
            break;
    }
}

const pressable_obj_t * p_keyboard_get_key(struct p_keyboard *kb, enum p_keyboard_keycode code)
{
    u_check_params(kb != NULL && code >= 0 && code < P_KEYBOARD_N_KEYS);
    return &kb->keys[code];
}

void p_keyboard_destroy(struct p_keyboard *kb)
{
    if (kb == NULL) return;

    s_log_debug("Destroying keyboard (mode \"%s\")...", keyboard_mode_strings[kb->mode]);
    switch(kb->mode) {
        case KB_MODE_EVDEV:
            evdev_keyboard_destroy(&kb->evdev);
            break;
        case KB_MODE_TTY:
            tty_keyboard_destroy(&kb->tty);
            break;
        case KB_MODE_X11:
            X11_keyboard_destroy(&kb->x11);
            break;
        default: case KB_MODE_FAIL:
            break;
    }

    free(kb);
}

#undef KB_MODES_LIST
