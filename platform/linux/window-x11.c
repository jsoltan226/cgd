#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <stdlib.h>
#include "core/log.h"
#include "core/shapes.h"
#include "core/util.h"
#include "core/librtld.h"
#include "../window.h"
#define P_INTERNAL_GUARD__
#include "window-x11.h"
#undef P_INTERNAL_GUARD__

#define MODULE_NAME "window-x11"

#define X11_LIB_NAME "libX11.so.6"

#define X11_SYM_LIST                                                            \
    X_(Display *, XOpenDisplay, const char *display_name)                       \
    X_(Window, XCreateSimpleWindow,                                             \
        Display *dpy, Window parent,                                            \
        int x, int y, unsigned int width, unsigned int height,                  \
        unsigned int border_width, unsigned long border, unsigned long bg       \
    )                                                                           \
    X_(int, XCloseDisplay, Display *dpy)                                        \
    X_(int, XSelectInput, Display *display, Window w, long event_mask)          \
    X_(int, XMapWindow, Display *dpy, Window w)                                 \
    X_(int, XNextEvent, Display *display, XEvent *event_return)                 \
    X_(XSizeHints *, XAllocSizeHints, void)                                     \
    X_(void, XSetWMNormalHints, Display *display, Window w, XSizeHints *hints)  \
    X_(void, XFree, void *data)                                                 \
    X_(int, XGetErrorText, Display *dpy, int code, char *ret_buf, int buf_size) \
    X_(Status, XGetWMNormalHints, Display *display, Window w,                   \
        XSizeHints *hints_return, long *supplied_return                         \
    )                                                                           \
    X_(int, XChangeProperty, Display *display, Window w,                        \
        Atom property, Atom type, int format, int  mode,                        \
        _Xconst unsigned char *data, int nelements                              \
    )                                                                           \
    X_(Atom, XInternAtom,                                                       \
        Display *display, _Xconst char *atom_name, Bool only_if_exists          \
    )                                                                           \
    X_(int, XSetErrorHandler, int (*handler)(Display *, XErrorEvent *))         \
    X_(int, XFlush, Display *dpy)                                               \
    X_(int, XDisplayWidth, Display *dpy, int screen_number)                     \
    X_(int, XDisplayHeight, Display *dpy, int screen_number)                    \


static struct lib *X11_lib = NULL;

#define X_(ret_type, name, ...) ret_type (*name) (__VA_ARGS__);
static struct {
    X11_SYM_LIST
    } X = { 0 };
#undef X_

static i32 dlopen_libX11();
/* static i32 libX11_error_handler(Display *dpy, XErrorEvent *ev); */

i32 window_X11_open(struct window_x11 *win,
    const unsigned char *title, const rect_t *area, const u32 flags)
{
    XSizeHints *hints = NULL;

    win->closed = false;

    /* Only try loading libX11 if it wasn't already loaded */
    if (X11_lib == NULL && dlopen_libX11()) {
        /* Don't use `goto_error` as all cleanup done in `err`
         * depends on libX11 being loaded */
        s_log_error("Failed to dlopen() libX11!");
        return -1;
    }

    /* Open connection with X server */
    win->dpy = X.XOpenDisplay(NULL);
    if (win->dpy == NULL)
        goto_error("Failed to connect to X server");

    /* Get screen parameters */
    win->screen_w = X.XDisplayWidth(win->dpy, win->scr);
    win->screen_h = X.XDisplayHeight(win->dpy, win->scr);

    /* Get the root window ("background") - our window will be a child of it */
    win->root = DefaultRootWindow(win->dpy);

    /* Get the screen on which our window will spawn */
    win->scr = DefaultScreen(win->dpy);

    i32 x = area->x, y = area->y, w = area->w, h = area->h;

    /* Handle WINDOW_POS_CENTERED flags */
    if (flags & P_WINDOW_POS_CENTERED_X)
        x = abs((i32)win->screen_w - w) / 2;

    if (flags & P_WINDOW_POS_CENTERED_Y)
        y = abs((i32)win->screen_h - h) / 2;

    /* Create the window */
    win->win = X.XCreateSimpleWindow(win->dpy, win->root, x, y, w, h, 1, 
        WhitePixel(win->dpy, win->scr), BlackPixel(win->dpy, win->scr));

    /* Enable events (e.g. keyboard input) */
    X.XSelectInput(win->dpy, win->win, ExposureMask | KeyPressMask);

    /* Make the window non-resizeable */
    hints = X.XAllocSizeHints();
    s_assert(hints != NULL, "Failed to allocate XSizeHints");

    i64 supplied_return = 0;
    if (X.XGetWMNormalHints(win->dpy, win->win, hints, &supplied_return) == 0)
        goto_error("Failed to get Window Manager hints");

    hints->min_width = hints->max_width = w;
    hints->min_height = hints->max_height = h;
    hints->flags |= (PMinSize | PMaxSize);

    X.XSetWMNormalHints(win->dpy, win->win, hints);
    X.XFree(hints);

    /* Set the window to floating */
    Atom NET_WM_STATE_ABOVE = X.XInternAtom(
        win->dpy, "_NET_WM_STATE_ABOVE", false
    );
    u8 NET_WM_STATE_ABOVE_state = 1;
    X.XChangeProperty(win->dpy, win->win,
        NET_WM_STATE_ABOVE, XA_INTEGER, 8, PropModeReplace,
        &NET_WM_STATE_ABOVE_state, 1
    );

    /* Set the window title */
    if (title != NULL) {
        Atom NET_WM_NAME = X.XInternAtom(
            win->dpy, "_NET_WM_NAME", false
            );
        X.XChangeProperty(win->dpy, win->win,
            NET_WM_NAME, XA_STRING, 8, PropModeReplace,
            title, strlen((const char *)title));

        Atom WM_NAME = X.XInternAtom(
            win->dpy, "WM_NAME", false
            );
        X.XChangeProperty(win->dpy, win->win,
            WM_NAME, XA_STRING, 8, PropModeReplace,
            title, strlen((const char *)title));
    }

    /* "Show" the window (if we don't do this it will be hidden) */
    X.XMapWindow(win->dpy, win->win);

    /* Flush all the commands */
    X.XFlush(win->dpy);

    s_log_debug("%s() OK; Screen is %ux%u", __func__,
        win->screen_w, win->screen_h);

    return 0;

err:
    if (hints != NULL) X.XFree(hints);
    window_X11_close(win);
    return 1;
}

void window_X11_close(struct window_x11 *win)
{
    if (win == NULL || win->closed) return;

    X.XCloseDisplay(win->dpy);
    if (X11_lib != NULL) librtld_close(X11_lib);
    win->closed = true;
}

void window_X11_render(struct window_x11 *x11,
    const pixel_t *data, const rect_t *area)
{
    s_log_warn("not implemented yet");
}

static i32 dlopen_libX11()
{
    static const char *sym_names[] = {
#define X_(ret_type, name, ...) #name,
        X11_SYM_LIST
#undef X_
        NULL
    };
    X11_lib = librtld_load(X11_LIB_NAME, sym_names);
    if (X11_lib == NULL)
        return 1;
    
#define X_(ret_type, name, ...) X.name = librtld_get_sym_handle(X11_lib, #name);
    X11_SYM_LIST
#undef X_

    return 0;
}

/*
static i32 libX11_error_handler(Display *dpy, XErrorEvent *ev)
{
    char buf[u_BUF_SIZE] = { 0 };
    s_log_error("sus: %s", X.XGetErrorText(dpy, ev->error_code, buf, u_BUF_SIZE - 1));
    return 0;
}
*/
