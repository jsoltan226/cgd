#ifndef LIBX11_RTLD_H_
#define LIBX11_RTLD_H_

#ifndef P_INTERNAL_GUARD__
#error This header file is internal to the cgd platform module and is not intended to be used elsewhere
#endif /* P_INTERNAL_GUARD__ */

#include <core/int.h>
#include <stdbool.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>

#define X11_LIB_NAME "libX11.so.6"

#define X11_SYM_LIST                                                            \
    X_(Display *, XOpenDisplay, const char *display_name)                       \
    X_(int, XCloseDisplay, Display *dpy)                                        \
    X_(int, XMapWindow, Display *dpy, Window w)                                 \
    X_(XSizeHints *, XAllocSizeHints, void)                                     \
    X_(void, XSetWMNormalHints, Display *display, Window w, XSizeHints *hints)  \
    X_(void, XFree, void *data)                                                 \
    X_(int, XChangeProperty,                                                    \
        Display *display, Window w, Atom property, Atom type,                   \
        int format, int mode, _Xconst unsigned char *data, int nelements        \
    )                                                                           \
    X_(Atom, XInternAtom,                                                       \
        Display *display, _Xconst char *atom_name, Bool only_if_exists          \
    )                                                                           \
    X_(int, XFlush, Display *dpy)                                               \
    X_(int, XDisplayWidth, Display *dpy, int screen_number)                     \
    X_(int, XDisplayHeight, Display *dpy, int screen_number)                    \
    X_(Status, XMatchVisualInfo,                                                \
        Display *display, int screen, int depth, int class,                     \
        XVisualInfo *vinfo_return                                               \
    )                                                                           \
    X_(Window, XCreateWindow,                                                   \
        Display *display, Window parent,                                        \
        int x, int y, unsigned int width, unsigned int height,                  \
        unsigned int border_width, int depth,                                   \
        unsigned int class, Visual *visual,                                     \
        unsigned long valuemask, XSetWindowAttributes *attributes               \
    )                                                                           \
    X_(int, XDestroyWindow, Display *display, Window w)                         \
    X_(int, XPutImage,                                                          \
        Display *display, Drawable d, GC gc, XImage *image,                     \
        int src_x, int src_y, int dest_x, int dest_y,                           \
        unsigned int width, unsigned int height                                 \
    )                                                                           \
    X_(int, XSetErrorHandler, int(*handler)(Display *dpy, XErrorEvent *ev))     \
    X_(int, XSetIOErrorHandler, int(*handler)(Display *dpy))                    \
    X_(char *, XDisplayName, _Xconst char *string)                              \
    X_(int, XGetErrorDatabaseText,                                              \
        Display *display, _Xconst char *name, _Xconst char *message,            \
        _Xconst char *default_string, char *buffer_return, int length           \
    )                                                                           \
    X_(int, XSync, Display *dpy, Bool discard)                                  \
    X_(KeySym, XLookupKeysym, XKeyEvent *key_event, int index)                  \
    X_(Status, XSetWMProtocols,                                                 \
        Display *display, Window w, Atom *protocols, int count                  \
    )                                                                           \
    X_(Bool, XCheckTypedWindowEvent,                                            \
        Display *display, Window w, int event_type, XEvent *event_return        \
    )                                                                           \
    X_(Bool, XCheckWindowEvent,                                                 \
        Display *display, Window w, long event_mask, XEvent *event_return       \
    )                                                                           \
    X_(Status, XInitImage, XImage *image)                                       \
    X_(GC, XCreateGC,                                                           \
        Display *display, Drawable d, unsigned long valuemask, XGCValues *vals  \
    )                                                                           \
    X_(int, XFreeGC, Display *display, GC gc)                                   \

/*
    X_(GC, XDefaultGC, Display *dpy, int scr)                                   \
    X_(XImage *, XCreateImage,                                                  \
        Display *display, Visual *visual, unsigned int depth, int format,       \
        int offset, char *data, unsigned int width, unsigned int height,        \
        int bitmap_pad, int bytes_per_line                                      \
    )                                                                           \
    X_(int, XGetErrorText, Display *dpy, int code, char *ret_buf, int buf_size) \
    X_(int, XNextEvent, Display *display, XEvent *event_return)                 \
    X_(int, XWindowEvent, Display *display, XEvent *event_return)               \
    X_(int, XPeekEvent, Display *display, XEvent *event_return)                 \
    X_(Bool, XCheckMaskEvent,                                                   \
        Display *display, long event_mask, XEvent *event_return                 \
    )                                                                           \
    X_(int, XGrabKeyboard,                                                      \
        Display *display, Window grab_window, Bool owner_events,                \
        int pointer_mode, int keyboard_mode, Time time                          \
    )                                                                           \
    X_(int, XUngrabKeyboard, Display *display, Time time)                       \
*/

#define X_(ret_type, name, ...) ret_type (*name) (__VA_ARGS__);
struct libX11 {
    X11_SYM_LIST
};
#undef X_

/* Returns 0 on success or if libX11 was already loaded, and 1 on failure */
i32 libX11_load(struct libX11 *o);
void libX11_unload();

/** Basically all the same, but for the libXext library **/

#define XEXT_LIB_NAME "libXext.so.6"
#define XEXT_SYM_LIST                                                           \
    X_(Bool, XShmQueryExtension, Display *display)                              \
    X_(XImage *, XShmCreateImage,                                               \
        Display *display, Visual *visual, unsigned int depth, int format,       \
        char *data, XShmSegmentInfo *shminfo,                                   \
        unsigned int width, unsigned int height                                 \
    )                                                                           \
    X_(Bool, XShmAttach, Display *display, XShmSegmentInfo *shminfo)            \
    X_(Bool, XShmDetach, Display *display, XShmSegmentInfo *shminfo)            \
    X_(Bool, XShmPutImage,                                                      \
        Display *display, Drawable d, GC gc, XImage *image,                     \
        int src_x, int src_y, int dest_x, int dest_y,                           \
        unsigned int width, unsigned int height, bool send_event                \
    )                                                                           \

#define X_(ret_type, name, ...) ret_type (*name) (__VA_ARGS__);
struct libXExt {
    XEXT_SYM_LIST
};
#undef X_

i32 libXExt_load(struct libXExt *o);
void libXExt_unload();

#endif /* LIBX11_RTLD_H_ */
