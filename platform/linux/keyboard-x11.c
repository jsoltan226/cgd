#include "core/int.h"
#include "../keyboard.h"
#include "core/log.h"
#include "core/pressable-obj.h"
#include "core/util.h"
#include <X11/X.h>
#include <X11/Xlib.h>
#include <pthread.h>
#include <string.h>

#define P_INTERNAL_GUARD__
#include "keyboard-x11.h"
#undef P_INTERNAL_GUARD__
#define P_INTERNAL_GUARD__
#include "window-x11.h"
#undef P_INTERNAL_GUARD__
#define P_INTERNAL_GUARD__
#include "libx11_rtld.h"
#undef P_INTERNAL_GUARD__

#define MODULE_NAME "keyboard-x11"

i32 keyboard_X11_init(struct keyboard_x11 *kb, struct window_x11 *win)
{
    u_check_params(win != NULL);
    memset(kb, 0, sizeof(struct keyboard_x11));

    if (libX11_load(&kb->Xlib))
        goto_error("Failed to load libX11!");

    kb->dpy = win->dpy;

    return 0;

err:
    keyboard_X11_destroy(kb);
    return 1;
}

void keyboard_X11_update_all_keys(struct keyboard_x11 *kb,
    pressable_obj_t pobjs[P_KEYBOARD_N_KEYS])
{
    bool key_updated[P_KEYBOARD_N_KEYS] = { 0 };
    XEvent ev = { 0 };
    while (kb->Xlib.XCheckMaskEvent(
            kb->dpy, KeyPressMask | KeyReleaseMask, &ev
        )
    ) {
        enum p_keyboard_keycode code = -1;
        for (u32 i = 0; i < P_KEYBOARD_N_KEYS; i++) {
            s_log_debug("Pressed key 0x00%x", ev.xkey.keycode);
            if (ev.xkey.keycode == x11_keycode_2_p_keycode_map[i][0]) {
                code = x11_keycode_2_p_keycode_map[i][1];
                break;
            }
        }

        if (code == -1)
            continue;

        pressable_obj_update(&pobjs[code], true);
        key_updated[code] = true;
    }

    /* This is basically the same as in `devinput_update_all_keys()`
     * in keyboard-devinput.c. Look there for explanations.
     */
    for (u32 i = 0; i < P_KEYBOARD_N_KEYS; i++) {
        if (!key_updated[i] && (pobjs[i].pressed || pobjs[i].down))
            pressable_obj_update(&pobjs[i], true);
    }
}

void keyboard_X11_destroy(struct keyboard_x11 *kb)
{
    if (kb == NULL) return;

    /* Don't try to unload libX11 herem
     * it should be done in p_window_close()! */
}
