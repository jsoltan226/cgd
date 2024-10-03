#include "../keyboard.h"
#include <core/int.h>
#include <core/pressable-obj.h>
#include <core/util.h>
#include <string.h>
#include <pthread.h>
#include <X11/Xlib.h>

#define P_INTERNAL_GUARD__
#include "keyboard-x11.h"
#undef P_INTERNAL_GUARD__
#define P_INTERNAL_GUARD__
#include "window-x11.h"
#undef P_INTERNAL_GUARD__
#define P_INTERNAL_GUARD__
#include "libx11-rtld.h"
#undef P_INTERNAL_GUARD__

#define MODULE_NAME "keyboard-x11"

i32 X11_keyboard_init(struct keyboard_x11 *kb, struct window_x11 *win)
{
    u_check_params(win != NULL);
    memset(kb, 0, sizeof(struct keyboard_x11));

    if (libX11_load(&kb->Xlib))
        goto_error("Failed to load libX11!");

    kb->win = win;
    
    /*
#define GRAB_WINDOW win->root
#define OWNER_EVENTS True
#define POINTER_GRAB_MODE GrabModeAsync
#define KEYBOARD_GRAB_MODE GrabModeAsync
    (void)kb->Xlib.XGrabKeyboard(win->dpy, GRAB_WINDOW, OWNER_EVENTS,
        POINTER_GRAB_MODE, KEYBOARD_GRAB_MODE, CurrentTime);
        */

    return 0;

err:
    X11_keyboard_destroy(kb);
    return 1;
}

void X11_keyboard_update_all_keys(struct keyboard_x11 *kb,
    pressable_obj_t pobjs[P_KEYBOARD_N_KEYS])
{
    XEvent ev = { 0 };
    bool key_updated[P_KEYBOARD_N_KEYS] = { 0 };
    
    /*
    while (kb->Xlib.XCheckWindowEvent(
            kb->win->dpy, kb->win->win, KeyPressMask | KeyReleaseMask, &ev
        )
    ) {
        enum p_keyboard_keycode code = -1;
        KeySym sym = kb->Xlib.XLookupKeysym(&ev.xkey, 0);
        for (u32 i = 0; i < P_KEYBOARD_N_KEYS; i++) {
            if (sym == libX11_keycode_map[i][0]) {
                code = libX11_keycode_map[i][1];
                break;
            }
        }

        if (code == -1)
            continue;
    */

        /* ev.type == KeyPress -> update with `true`
         * ev.type == KeyRelease (or anything else) -> update with `false`
         */
    /*
        pressable_obj_update(&pobjs[code], ev.type == KeyPress);
        key_updated[code] = true;
    }

    */
    /* This is basically the same as in `evdev_keyboard_update_all_keys()`
     * in keyboard-evdev.c. Look there for explanations.
     */
    /*
    for (u32 i = 0; i < P_KEYBOARD_N_KEYS; i++) {
        if (!key_updated[i] && (pobjs[i].pressed || pobjs[i].down))
            pressable_obj_update(&pobjs[i], true);
    }
    */
}

void X11_keyboard_destroy(struct keyboard_x11 *kb)
{
    if (kb == NULL) return;

    /*
    kb->Xlib.XUngrabKeyboard(kb->win->dpy, CurrentTime);
    */

    /* Don't try to unload libX11 here
     * it should be done in p_window_close()! */
}
