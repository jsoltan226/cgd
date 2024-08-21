#define MODULE_NAME "window"
#include "core/util.h"
#include "core/librtld.h"
#define P_INTERNAL_GUARD__
#include "window-x11.h"
#undef P_INTERNAL_GUARD__

#define X11_LIB_NAME "libX11.so.6"

#define X11_SYM_LIST        \
    X_(XOpenDisplay)        \
    X_(XCreateSimpleWindow) \
    X_(XCloseDisplay)       \
    X_(XSelectInput)        \
    X_(XMapWindow)          \
    X_(XNextEvent)          \


static struct lib *X11_lib = NULL;
static struct {
    Display *(*XOpenDisplay)(const char *);
    Window (*XCreateSimpleWindow)(Display *, Window, 
        int, int, unsigned int, unsigned int,
        unsigned int,
        unsigned long, unsigned long
    );
    int (*XCloseDisplay)(Display *);
    int (*XSelectInput)(Display *, Window, long);
    int (*XMapWindow)(Display *, Window);
    int (*XNextEvent)(Display *, XEvent *);
} X = { 0 };

static i32 dlopen_libX11();

i32 window_X11_open(struct window_x11 *x11, i32 x, i32 y, i32 w, i32 h)
{
    if (dlopen_libX11())
        goto_error("Failed to dlopen() libX11!");

    x11->dpy = X.XOpenDisplay(NULL);
    if (x11->dpy == NULL)
        goto_error("Failed to connect to X server!");

    x11->root = DefaultRootWindow(x11->dpy);
    x11->scr = DefaultScreen(x11->dpy);
    x11->win = X.XCreateSimpleWindow(x11->dpy, x11->root, x, y, w, h, 1, 
        BlackPixel(x11->dpy, x11->scr), WhitePixel(x11->dpy, x11->scr));

    X.XSelectInput(x11->dpy, x11->win, ExposureMask | KeyPressMask);
    X.XMapWindow(x11->dpy, x11->win);

    /* Wait for the window to open ("Expose" - become visible) */
    XEvent e;
    while (X.XNextEvent(x11->dpy, &e), e.type != Expose)
        ;

    return 0;

err:
    window_X11_close(x11);
    return 1;
}

void window_X11_close(struct window_x11 *x11)
{
    if (x11 == NULL) return;
    s_log_debug("Closing X11 window...");
    X.XCloseDisplay(x11->dpy);
}

static i32 dlopen_libX11()
{
    static const char *sym_names[] = {
#define X_(name) #name,
        X11_SYM_LIST
#undef X_
        NULL
    };
    X11_lib = librtld_load(X11_LIB_NAME, sym_names);
    if (X11_lib == NULL)
        return 1;
    
#define X_(name) X.name = librtld_get_sym_handle(X11_lib, #name);
    X11_SYM_LIST
#undef X_

    return 0;
}
