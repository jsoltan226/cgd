#include <X11/X.h>
#include <X11/Xfuncproto.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <string.h>
#include <pthread.h>
#include "core/log.h"
#include "core/math.h"
#include "core/pixel.h"
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
    X_(int, XCloseDisplay, Display *dpy)                                        \
    X_(int, XMapWindow, Display *dpy, Window w)                                 \
    X_(int, XNextEvent, Display *display, XEvent *event_return)                 \
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
    X_(GC, XDefaultGC, Display *dpy, int scr)                                   \
    X_(XImage *, XCreateImage,                                                  \
        Display *display, Visual *visual, unsigned int depth, int format,       \
        int offset, char *data, unsigned int width, unsigned int height,        \
        int bitmap_pad, int bytes_per_line                                      \
    )                                                                           \
    X_(int, XDestroyWindow, Display *display, Window w)                         \
    X_(int, XPutImage,                                                          \
        Display *display, Drawable d, GC gc, XImage *image,                     \
        int src_x, int src_y, int dest_x, int dest_y,                           \
        unsigned int width, unsigned int height                                 \
    )                                                                           \
    X_(int, XSetErrorHandler, int(*handler)(Display *dpy, XErrorEvent *ev))     \
    X_(int, XGetErrorText, Display *dpy, int code, char *ret_buf, int buf_size) \
    X_(int, XSetIOErrorHandler, int(*handler)(Display *dpy))                    \
    X_(char *, XDisplayName, _Xconst char *string)                              \
    X_(int, XGetErrorDatabaseText,                                              \
        Display *display, _Xconst char *name, _Xconst char *message,            \
        _Xconst char *default_string, char *buffer_return, int length           \
    )                                                                           \
    X_(int, XSync, Display *dpy, Bool discard)                                  \

static struct lib *X11_lib = NULL;
static bool libX11_error = false;
static pthread_mutex_t libX11_error_mutex = PTHREAD_MUTEX_INITIALIZER;

#define X_(ret_type, name, ...) ret_type (*name) (__VA_ARGS__);
static struct {
    X11_SYM_LIST
    } X = { 0 };
#undef X_

static i32 dlopen_libX11();
static i32 libX11_error_handler(Display *dpy, XErrorEvent *ev);
static noreturn i32 libX11_IO_error_handler(Display *dpy);

i32 window_X11_open(struct window_x11 *win,
    const unsigned char *title, const rect_t *area, const u32 flags)
{
    pthread_mutex_lock(&libX11_error_mutex);

    memset(win, 0, sizeof(struct window_x11));
    XSizeHints *hints = NULL;

    win->closed = false;
    win->bad_window = false;

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

    /* Set error handlers */
    X.XSetErrorHandler(libX11_error_handler);
    X.XSetIOErrorHandler(libX11_IO_error_handler);

    /* Get screen parameters */
    win->screen_w = X.XDisplayWidth(win->dpy, win->scr);
    win->screen_h = X.XDisplayHeight(win->dpy, win->scr);

    /* Get the root window ("background") - our window will be a child of it */
    win->root = DefaultRootWindow(win->dpy);

    /* Get the screen on which our window will spawn */
    win->scr = DefaultScreen(win->dpy);

    /* Get the Graphics Context */
    win->gc = X.XDefaultGC(win->dpy, win->scr);

    /* Obtain display visual info */
#define SCREEN_DEPTH 24
    if (X.XMatchVisualInfo(
            win->dpy, win->scr, SCREEN_DEPTH, TrueColor, &win->vis_info
        ) == 0
    ) {
        goto_error("No matching screen visual mode");
    }

    i32 x = area->x, y = area->y, w = area->w, h = area->h;

    /* Handle WINDOW_POS_CENTERED flags */
    if (flags & P_WINDOW_POS_CENTERED_X)
        x = abs((i32)win->screen_w - w) / 2;

    if (flags & P_WINDOW_POS_CENTERED_Y)
        y = abs((i32)win->screen_h - h) / 2;

    /* Create the window */
#define BORDER_W 0
#define WINDOW_CLASS InputOutput

#define ATTR_VALUE_MASK (CWEventMask)
    XSetWindowAttributes attr = { 0 };
    attr.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask;

    win->win = X.XCreateWindow(win->dpy, win->root, x, y, w, h,
        BORDER_W, win->vis_info.depth, WINDOW_CLASS, win->vis_info.visual,
        ATTR_VALUE_MASK, &attr);

    X.XSync(win->dpy, true);
    if (libX11_error) {
        win->bad_window = true;
        goto err;
    }

    /* Make the window non-resizeable */
    hints = X.XAllocSizeHints();
    s_assert(hints != NULL, "Failed to allocate XSizeHints");

    hints->min_width = hints->max_width = w;
    hints->min_height = hints->max_height = h;
    hints->flags |= (PMinSize | PMaxSize);

    X.XSetWMNormalHints(win->dpy, win->win, hints);
    if (libX11_error) goto err;

    X.XFree(hints);

    /* Set the window to floating */
    Atom NET_WM_STATE_ABOVE = X.XInternAtom(
        win->dpy, "_NET_WM_STATE_ABOVE", false
    );
    if (libX11_error) goto err;

    u8 NET_WM_STATE_ABOVE_state = 1;
    X.XChangeProperty(win->dpy, win->win,
        NET_WM_STATE_ABOVE, XA_INTEGER, 8, PropModeReplace,
        &NET_WM_STATE_ABOVE_state, 1
    );
    if (libX11_error) goto err;

    /* Set the window title */
    if (title != NULL) {
        Atom NET_WM_NAME = X.XInternAtom(win->dpy, "_NET_WM_NAME", false);
        if (libX11_error) goto err;

        X.XChangeProperty(win->dpy, win->win,
            NET_WM_NAME, XA_STRING, 8, PropModeReplace,
            title, strlen((const char *)title));
        if (libX11_error) goto err;

        Atom WM_NAME = X.XInternAtom(win->dpy, "WM_NAME", false);
        if (libX11_error) goto err;

        X.XChangeProperty(win->dpy, win->win,
            WM_NAME, XA_STRING, 8, PropModeReplace,
            title, strlen((const char *)title));
        if (libX11_error) goto err;
    }

    /* "Show" the window (if we don't do this it will be hidden) */
    if(X.XMapWindow(win->dpy, win->win), libX11_error) goto err;

    /* Initialize the framebuffer */
    win->data.w = area->w;
    win->data.h = area->h;
    win->data.buf = calloc(area->w * area->h, sizeof(pixel_t));
    s_assert(win->data.buf != NULL, "calloc() failed for pixel data");

    /* Create the X Image and bind it with our framebuffer */
#define BIT_DEPTH 32
#define BUF_OFFSET 0
#define XIMAGE_FORMAT ZPixmap
#define XIMAGE_BITMAP_PAD 0
#define BYTES_PER_PIXEL (BIT_DEPTH / 8)

    win->Ximg = X.XCreateImage(win->dpy, win->vis_info.visual,
        win->vis_info.depth, XIMAGE_FORMAT, BUF_OFFSET,
        (char *)win->data.buf, win->data.w, win->data.h,
        BIT_DEPTH, XIMAGE_BITMAP_PAD
    );
    if (win->Ximg == NULL) {
        goto_error("Failed to create X11 image");
    } else if (libX11_error) {
        goto err;
    }

    /* Flush all the commands */
    X.XFlush(win->dpy);

    s_log_debug("%s() OK; Screen is %ux%u", __func__,
        win->screen_w, win->screen_h);

    pthread_mutex_unlock(&libX11_error_mutex);
    return 0;

err:
    libX11_error = false;
    if (hints != NULL) X.XFree(hints);
    /* window_X11_close() will later be called by p_window_close();
     * no need to do it here */
    pthread_mutex_unlock(&libX11_error_mutex);
    return 1;
}

void window_X11_close(struct window_x11 *win)
{
    if (win == NULL || win->closed) return;

    s_log_debug("Destroying X11 window...");

    /* We don't need to free win->data.buf if win->Ximg is initialized -
     * From the Xlib manual:
     *
     * Note that when the image is created using XCreateImage, XGetImage,
     * or XSubImage, the destroy procedure that the XDestroyImage function calls
     * frees both the image structure and 
     * the data pointed to by the image structure.
     */
    if (win->Ximg != NULL)
        XDestroyImage(win->Ximg);
    else if (win->data.buf != NULL)
        free(win->data.buf);

    if (X11_lib != NULL) {
        if (!win->bad_window)
            X.XDestroyWindow(win->dpy, win->win);
        X.XCloseDisplay(win->dpy);

        librtld_close(X11_lib);
    }

    win->closed = true;
}

void window_X11_render(struct window_x11 *win,
    const pixel_t *data, const rect_t *area)
{
#define SRC_X 0
#define SRC_Y 0
#define DST_X 0
#define DST_Y 0
    memcpy(win->data.buf, data,
        u_min(win->data.w * win->data.h, area->w * area->h) * sizeof(pixel_t)
    );

    X.XPutImage(win->dpy, win->win, win->gc, win->Ximg,
        SRC_X, SRC_Y, DST_X, DST_Y, win->data.w, win->data.h);
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

static i32 libX11_error_handler(Display *dpy, XErrorEvent *ev)
{
#define ERR_MSG_BUF_SIZE 512
#define DEFAULT_STR "<unknown>"


    /* Get the name of the function that failed */
    char opcode_str[32] = { 0 };
    snprintf(opcode_str, sizeof(opcode_str), "%i", ev->request_code);

    char function_name_buf[ERR_MSG_BUF_SIZE] = { 0 };
    X.XGetErrorDatabaseText(dpy, "XRequest", opcode_str, DEFAULT_STR,
        function_name_buf, ERR_MSG_BUF_SIZE);

    /* Get the description of the error */
    char error_code_str[32] = { 0 };
    snprintf(error_code_str, sizeof(error_code_str), "%i", ev->error_code);

    char error_description_buf[ERR_MSG_BUF_SIZE] = { 0 };
    X.XGetErrorDatabaseText(dpy, "XProtoError", error_code_str, DEFAULT_STR,
        error_description_buf, ERR_MSG_BUF_SIZE);


    s_log_error("An X11 error occured (in function \"%s\"): %s",
        function_name_buf, error_description_buf);
    libX11_error = true;
    return 0;
}

static noreturn i32 libX11_IO_error_handler(Display *dpy)
{
    s_log_fatal(MODULE_NAME, __func__,
        "Fatal X11 I/O error (display %s)",
        X.XDisplayName(NULL));
}
