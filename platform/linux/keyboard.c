#include "../keyboard.h"
#include <core/int.h>
#include <core/log.h>
#include <core/util.h>
#include <core/pressable-obj.h>
#include <stdlib.h>

#define P_INTERNAL_GUARD__
#include "keyboard-x11.h"
#undef P_INTERNAL_GUARD__
#define P_INTERNAL_GUARD__
#include "keyboard-tty.h"
#undef P_INTERNAL_GUARD__
#define P_INTERNAL_GUARD__
#include "keyboard-evdev.h"
#undef P_INTERNAL_GUARD__
#define P_INTERNAL_GUARD__
#include "window-internal.h"
#undef P_INTERNAL_GUARD__

#define MODULE_NAME "keyboard"

#define N_KEYBOARD_TYPES 4
#define KB_TYPES_LIST   \
    X_(KB_TYPE_FAIL)    \
    X_(KB_TYPE_EVDEV)   \
    X_(KB_TYPE_TTY)     \
    X_(KB_TYPE_X11)     \

#define X_(name) name,
enum keyboard_type {
    KB_TYPES_LIST
};
#undef X_

#define X_(name) #name,
static const char *keyboard_type_strings[N_KEYBOARD_TYPES] = {
    KB_TYPES_LIST
};
#undef X_

static const enum keyboard_type
fallback_types[N_WINDOW_TYPES][N_KEYBOARD_TYPES] = {
    [WINDOW_TYPE_FRAMEBUFFER] = {
        KB_TYPE_EVDEV,
        KB_TYPE_TTY,
        KB_TYPE_FAIL
    },
    [WINDOW_TYPE_X11] = {
        KB_TYPE_X11,
        KB_TYPE_FAIL
    },
    [WINDOW_TYPE_DUMMY] = {
        KB_TYPE_FAIL
    },
};

struct p_keyboard {
    enum keyboard_type type;
    
    union {
        struct keyboard_evdev evdev;
        struct keyboard_tty tty;
        struct keyboard_x11 x11;
    };

    pressable_obj_t keys[P_KEYBOARD_N_KEYS];
};

struct p_keyboard * p_keyboard_init(struct p_window *win)
{
    u_check_params(win != NULL);

    struct p_keyboard *kb = calloc(1, sizeof(struct p_keyboard));
    s_assert(kb != NULL, "calloc() failed for struct p_keyboard");

    enum window_type win_type;
#define DEFAULT_WINDOW_FALLBACK_TYPE WINDOW_TYPE_FRAMEBUFFER
    if (win == NULL) {
        s_log_warn("Cannot get window metadata. Assuming window type is %s",
            window_type_strings[DEFAULT_WINDOW_FALLBACK_TYPE]);
        win_type = DEFAULT_WINDOW_FALLBACK_TYPE;
    } else {
        win_type = win->type;
    }

    u32 i = 0;
    do {
        s_log_debug("Attempting keyboard init with type \"%s\"...",
            keyboard_type_strings[ fallback_types[win_type][i] ]
        );
        switch (fallback_types[win_type][i]) {
            case KB_TYPE_EVDEV:
                if (evdev_keyboard_init(&kb->evdev))
                    s_log_warn("Failed to set up keyboard using event devices");
                else
                    goto keyboard_setup_success;
                break;
            case KB_TYPE_TTY:
                if (tty_keyboard_init(&kb->tty))
                    s_log_warn("Failed to set up keyboard using tty stdin");
                else
                    goto keyboard_setup_success;
                break;
            case KB_TYPE_X11:
                if (X11_keyboard_init(&kb->x11, &win->x11))
                    s_log_warn("Failed to set up keyboard with X11");
                else
                    goto keyboard_setup_success;
                break;
            default: case KB_TYPE_FAIL:
                goto_error("Failed to set up keyboard in any known way");
                break;
        };
    } while (++i < N_KEYBOARD_TYPES);

keyboard_setup_success:
    kb->type = fallback_types[win_type][i];
    s_log_info("%s() OK, keyboard type is \"%s\"",
        __func__, keyboard_type_strings[kb->type]);

    return kb;

err:
    p_keyboard_destroy(&kb);
    return NULL;
}

void p_keyboard_update(struct p_keyboard *kb)
{
    u_check_params(kb != NULL);

    switch (kb->type) {
        case KB_TYPE_TTY:
            tty_keyboard_update_all_keys(&kb->tty, kb->keys);
            break;
        case KB_TYPE_EVDEV:
            evdev_keyboard_update_all_keys(&kb->evdev, kb->keys);
            break;
        case KB_TYPE_X11:
            X11_keyboard_update_all_keys(&kb->x11, kb->keys);
            break;
        default:
            break;
    }
}

const pressable_obj_t * p_keyboard_get_key(const struct p_keyboard *kb,
    enum p_keyboard_keycode code)
{
    u_check_params(kb != NULL && code >= 0 && code < P_KEYBOARD_N_KEYS);
    return &kb->keys[code];
}

void p_keyboard_destroy(struct p_keyboard **kb_p)
{
    if (kb_p == NULL || *kb_p == NULL) return;
    struct p_keyboard *kb = *kb_p;

    s_log_debug("Destroying keyboard (type \"%s\")...",
        keyboard_type_strings[kb->type]);
    switch(kb->type) {
        case KB_TYPE_EVDEV:
            evdev_keyboard_destroy(&kb->evdev);
            break;
        case KB_TYPE_TTY:
            tty_keyboard_destroy(&kb->tty);
            break;
        case KB_TYPE_X11:
            X11_keyboard_destroy(&kb->x11);
            break;
        default: case KB_TYPE_FAIL:
            break;
    }

    u_nzfree(kb_p);
}

#undef KB_TYPES_LIST
