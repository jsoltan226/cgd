#include "../mouse.h"
#include "core/log.h"
#include <core/int.h>
#include <core/pressable-obj.h>
#include <string.h>
#include <stdbool.h>
#include <X11/Xlib.h>
#define P_INTERNAL_GUARD__
#include "mouse-x11.h"
#undef P_INTERNAL_GUARD__
#define P_INTERNAL_GUARD__
#include "window-x11.h"
#undef P_INTERNAL_GUARD__
#define P_INTERNAL_GUARD__
#include "libx11-rtld.h"
#undef P_INTERNAL_GUARD__
#define P_INTERNAL_GUARD__
#include "mouse-internal.h"
#undef P_INTERNAL_GUARD__

#define MODULE_NAME "mouse-x11"

i32 mouse_X11_init(struct mouse_x11 *mouse, struct window_x11 *win, u32 flags)
{
    memset(mouse, 0, sizeof(struct mouse_x11));

    mouse->win = win;

    /*
    memcpy(&mouse->Xlib, &mouse->win->Xlib, sizeof(struct libX11));
    */
    return 0;
}

void mouse_X11_update(struct p_mouse *mouse)
{
    struct window_x11 *win = mouse->x11.win;
    struct libX11 *X = &mouse->x11.Xlib;
    XEvent ev;

    /*
    while (X->XCheckWindowEvent(win->dpy, win->win, 
            ButtonPressMask | ButtonReleaseMask | PointerMotionMask |
            EnterWindowMask | LeaveWindowMask, &ev)
    ) {
        if (ev.type == MotionNotify) {
            mouse->x = ev.xmotion.x;
            mouse->y = ev.xmotion.y;
            continue;
        } else if (ev.type == ButtonPress || ev.type == ButtonRelease) {
            enum p_mouse_button button;
            switch (ev.xbutton.button) {
                case Button1: button = P_MOUSE_BUTTON_LEFT; break;
                case Button2: button = P_MOUSE_BUTTON_MIDDLE; break;
                case Button3: button = P_MOUSE_BUTTON_RIGHT; break;
                default:
                    continue;
            }

            s_log_debug("button %i %s", button, ev.type == ButtonPress ? "press" : "release");
            pressable_obj_update(&mouse->buttons[button],
                    ev.type == ButtonPress);

        }
    }
    */
}

void mouse_X11_destroy(struct mouse_x11 *mouse)
{
}
