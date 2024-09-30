#define _GNU_SOURCE
#include "../event.h"
#include "../window.h"
#include <core/log.h>
#include <core/util.h>
#include <core/pixel.h>
#include <core/shapes.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdnoreturn.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/Xfuncproto.h>
#include <X11/extensions/XShm.h>
#define P_INTERNAL_GUARD__
#include "window-x11.h"
#undef P_INTERNAL_GUARD__
#define P_INTERNAL_GUARD__
#include "libx11-rtld.h"
#undef P_INTERNAL_GUARD__

#define MODULE_NAME "window-x11"

/* libX11 error handlers */
static i32 libX11_error_handler(Display *dpy, XErrorEvent *ev);
static noreturn i32 libX11_IO_error_handler(Display *dpy);

static i32 attach_shm(struct window_x11_shm *shm, Display *dpy, u64 size);
static void detach_shm(struct window_x11_shm *shm, Display *dpy);

/* Used by our X11 error handler */
static bool libX11_error = false;

/* Used to keep track of how many windows are open,
 * so that we call `libX11_unload()` only if there are 0 windows left */
static u32 n_open_windows = 0; 

/* For thread safety. Used to protect the above 2 variables */
static pthread_mutex_t libX11_mutex = PTHREAD_MUTEX_INITIALIZER;

static void * listening_thread_fn(void *arg);

i32 window_X11_open(struct window_x11 *win,
    const unsigned char *title, const rect_t *area, const u32 flags)
{
    memset(win, 0, sizeof(struct window_x11));

    /* Set up multi-threading */
    pthread_mutex_lock(&libX11_mutex);

    XSizeHints *hints = NULL;

    win->closed = false;
    win->bad_window = false;

    struct libX11 X;
    if (libX11_load(&X)) {
        /* Don't use `goto_error` as all cleanup done in `err`
         * depends on libX11 being loaded */
        s_log_error("Failed to load libX11!");
        return -1;
    }
    memcpy(&win->Xlib, &X, sizeof(struct libX11));

    /* Open connection with X server */
    win->dpy = X.XOpenDisplay(NULL);
    if (win->dpy == NULL)
        goto_error("Failed to connect to X server");

    /* Set error handlers */
    X.XSetErrorHandler(libX11_error_handler);
    X.XSetIOErrorHandler(libX11_IO_error_handler);

    /* Query and load extensions */
    struct libXExt Xe;
    if (libXExt_load(&Xe)) {
        win->shm.has_shm_extension = false;
    } else if (!Xe.XShmQueryExtension(win->dpy)) {
        win->shm.has_shm_extension = false;
        libXExt_unload();
    } else {
        win->shm.has_shm_extension = true;
        memcpy(&win->shm.XExt, &Xe, sizeof(struct libXExt));
    }

    /* Get screen parameters */
    win->screen_w = X.XDisplayWidth(win->dpy, win->scr);
    win->screen_h = X.XDisplayHeight(win->dpy, win->scr);

    /* Get the root window ("background") - our window will be a child of it */
    win->root = DefaultRootWindow(win->dpy);

    /* Get the screen on which our window will spawn */
    win->scr = DefaultScreen(win->dpy);

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

    if (flags & P_WINDOW_TYPE_DUMMY ) {
#define DUMMY_WINDOW_CLASS InputOnly

#define DUMMY_WINDOW_ATTR_VALUE_MASK (CWEventMask)
        XSetWindowAttributes attr = { 0 };
        attr.event_mask = KeyPressMask | KeyReleaseMask |
            PointerMotionMask | ButtonPressMask | ButtonReleaseMask |
            EnterWindowMask | LeaveWindowMask;

        win->win = X.XCreateWindow(win->dpy, win->root, x, y, w, h,
            BORDER_W, CopyFromParent, DUMMY_WINDOW_CLASS, NULL,
            DUMMY_WINDOW_ATTR_VALUE_MASK, &attr);

    } else {
#define WINDOW_CLASS InputOutput

#define ATTR_VALUE_MASK (CWEventMask)
        XSetWindowAttributes attr = { 0 };
        attr.event_mask = KeyPressMask | KeyReleaseMask |
            PointerMotionMask | ButtonPressMask | ButtonReleaseMask |
            EnterWindowMask | LeaveWindowMask;

        win->win = X.XCreateWindow(win->dpy, win->root, x, y, w, h,
            BORDER_W, win->vis_info.depth, WINDOW_CLASS, win->vis_info.visual,
            ATTR_VALUE_MASK, &attr);
    }

    /* Wait for the requests to be processed by the X server */
    X.XSync(win->dpy, true);
    if (libX11_error) {
        win->bad_window = true;
        goto err;
    }

    /* Enable handling of window-close events */
    win->WM_DELETE_WINDOW = X.XInternAtom(win->dpy, "WM_DELETE_WINDOW", false);
    if (libX11_error) goto err;

    if (X.XSetWMProtocols(win->dpy, win->win, &win->WM_DELETE_WINDOW, 1) == 0)
        goto_error("Failed to set WM_DELETE_WINDOW property");

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

    /* Start the event-listning thread */
    win->listener.running = true;
    if (pthread_create(&win->listener.thread, NULL, listening_thread_fn, win))
        goto_error("Failed to create listener thread: %s", strerror(errno));

    /* Anything beyond this point is related to graphics,
     * and we don't want that in a dummy window */
    if (flags & P_WINDOW_TYPE_DUMMY)
        goto ret;

    /* Create the Graphics Context */
#define GCV_MASK GCGraphicsExposures
    XGCValues gcv = {
        .graphics_exposures = 0
    };
    win->gc = X.XCreateGC(win->dpy, win->win, GCV_MASK, &gcv);
    if (libX11_error) goto err;

    /* "Show" the window (if we don't do this it will be hidden) */
    if (X.XMapWindow(win->dpy, win->win), libX11_error) goto err;

    /* Initialize the framebuffer to 0 */
    win->data = NULL;
    memset(&win->Ximg, 0, sizeof(XImage));

ret:
    /* Flush all the commands */
    X.XFlush(win->dpy);

    s_log_debug("%s() OK; Screen is %ux%u, use shm: %b", __func__,
        win->screen_w, win->screen_h, win->shm.attached);

    n_open_windows++;
    pthread_mutex_unlock(&libX11_mutex);
    return 0;

err:
    libX11_error = false;
    if (hints != NULL) X.XFree(hints);
    /* window_X11_close() will later be called by p_window_close();
     * no need to do it here */
    pthread_mutex_unlock(&libX11_mutex);
    return 1;
}

void window_X11_close(struct window_x11 *win)
{
    if (win == NULL || win->closed) return;

    s_log_debug("Destroying X11 window...");
    window_X11_unbind_fb(win);

    /* Delete the thread BEFORE closing the connection to X */
    win->listener.running = false;
    pthread_join(win->listener.thread, NULL);

    pthread_mutex_lock(&libX11_mutex);
    struct libX11 X;
    if (!libX11_load(&X)) {
        if (!win->bad_window) {
            X.XFreeGC(win->dpy, win->gc);
            X.XDestroyWindow(win->dpy, win->win);
        }
        X.XCloseDisplay(win->dpy);

        if (n_open_windows == 0) {
            libX11_unload();
            libXExt_unload();
        } else
            n_open_windows--;
    }
    pthread_mutex_unlock(&libX11_mutex);

    /* Avoid use-after-free */
    memset(win, 0, sizeof(struct window_x11));
    win->closed = true;
}

void window_X11_render(struct window_x11 *win)
{
    s_assert(win->data->buf != NULL,
        "Attempt to render an unintialized framebuffer");

#define SRC_X 0
#define SRC_Y 0
#define DST_X 0
#define DST_Y 0
    pthread_mutex_lock(&libX11_mutex);
    if (win->shm.attached) {
        win->shm.XExt.XShmPutImage(win->dpy, win->win, win->gc, &win->Ximg,
            SRC_X, SRC_Y, DST_X, DST_Y, win->data->w, win->data->h, False);
    } else {
        win->Xlib.XPutImage(win->dpy, win->win, win->gc, &win->Ximg,
            SRC_X, SRC_Y, DST_X, DST_Y, win->data->w, win->data->h);
    }
    win->Xlib.XSync(win->dpy, true);
    pthread_mutex_unlock(&libX11_mutex);
}

void window_X11_bind_fb(struct window_x11 *win, struct pixel_flat_data *fb)
{
    if (win->data != NULL) {
        s_log_error("A framebuffer is already bound to this window.");
        return;
    }
    win->data = fb;

    /* Create the X Image and bind it to our framebuffer */
    win->Ximg.width             = win->data->w;
    win->Ximg.height            = win->data->h;
    win->Ximg.xoffset           = 0;
    win->Ximg.format            = ZPixmap;
    win->Ximg.data              = (char *)win->data->buf;
    win->Ximg.byte_order        = LSBFirst;
    win->Ximg.bitmap_unit       = 32,
    win->Ximg.bitmap_bit_order  = LSBFirst;
    win->Ximg.bitmap_pad        = 32;
    win->Ximg.depth             = win->vis_info.depth;
    win->Ximg.bytes_per_line    = win->data->w * sizeof(pixel_t);
    win->Ximg.bits_per_pixel    = 32;
    win->Ximg.red_mask          = 0xff0000;
    win->Ximg.green_mask        = 0x00ff00;
    win->Ximg.blue_mask         = 0x0000ff;

    u64 size = win->data->w * win->data->h * sizeof(*win->data->buf);

    pthread_mutex_lock(&libX11_mutex);
    if (win->shm.has_shm_extension && !attach_shm(&win->shm, win->dpy, size)) {
        free(win->data->buf);
        win->data->buf = (pixel_t *)win->shm.info.shmaddr;
        win->Ximg.data = win->shm.info.shmaddr;

        XImage *tmp = win->shm.XExt.XShmCreateImage(win->dpy, win->vis_info.visual,
            win->vis_info.depth, ZPixmap, (char *)win->data->buf,
            &win->shm.info, win->data->w, win->data->h);
        if (tmp == NULL)
            s_log_fatal(MODULE_NAME, __func__, "Failed to create shm XImage!");

        memcpy(&win->Ximg, tmp, sizeof(XImage));
        XDestroyImage(tmp);

    } else {
        if (win->Xlib.XInitImage(&win->Ximg) == 0)
            s_log_fatal(MODULE_NAME, __func__, "Failed to create XImage!");
    }

    win->Xlib.XSync(win->dpy, true);
    pthread_mutex_unlock(&libX11_mutex);
}

void window_X11_unbind_fb(struct window_x11 *win)
{
    if (win->data != NULL) {
        if (win->shm.attached)
            detach_shm(&win->shm, win->dpy);
        else if (win->data->buf != NULL)
            free(win->data->buf);

        win->data->buf = NULL;

        memset(&win->Ximg, 0, sizeof(XImage));

        win->data = NULL;
    }
}

static i32 libX11_error_handler(Display *dpy, XErrorEvent *ev)
{
#define ERR_MSG_BUF_SIZE 512
#define DEFAULT_STR "<unknown>"
    /* If this function got called,
     * we can safely assume that libX11 has already been loaded */

    pthread_mutex_lock(&libX11_mutex);
    struct libX11 X;
    (void) libX11_load(&X);


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

    pthread_mutex_unlock(&libX11_mutex);

    s_log_error("An X11 error occured (in call to \"%s\"): %s",
        function_name_buf, error_description_buf);
    libX11_error = true;
    return 0;
}

static noreturn i32 libX11_IO_error_handler(Display *dpy)
{
    /* Same as above */
    struct libX11 X;
    (void) libX11_load(&X);

    pthread_mutex_lock(&libX11_mutex);
    s_log_fatal(MODULE_NAME, __func__,
        "Fatal X11 I/O error (display %s)",
        X.XDisplayName(NULL));
}

static void * listening_thread_fn(void *arg)
{
    struct window_x11 *win = (struct window_x11 *)arg;

    XEvent ev;
    /* Check for WM_DELETE_WINDOW message */
    while (win->listener.running) {
        pthread_mutex_lock(&libX11_mutex);
        while (win->Xlib.XCheckTypedWindowEvent(
                    win->dpy, win->win, ClientMessage, &ev
                    )) {
            if (ev.xclient.data.l[0] == win->WM_DELETE_WINDOW) {
                const struct p_event quit_ev = { .type = P_EVENT_QUIT };
                p_event_send(&quit_ev);
            }
        }
        pthread_mutex_unlock(&libX11_mutex);
        usleep(10000); /* sleep for 10 ms */
    }

    pthread_exit(NULL);
}

static i32 attach_shm(struct window_x11_shm *shm, Display *dpy, u64 size)
{
    shm->info.shmid = shmget(IPC_PRIVATE, size, IPC_CREAT | 0600);
    if (shm->info.shmid == -1)
        goto_error("Failed to create shared memory block for XImage: %s. "
                "Falling back to XPutImage().", strerror(errno));

    shm->info.shmaddr = shmat(shm->info.shmid, NULL, 0);
    if (shm->info.shmaddr == (void *)-1)
        goto_error("Failed to map shared memory for XImage: %s. "
            "Falling back to XPutImage().", strerror(errno));

    shm->info.shmseg = shm->info.shmid;
    shm->info.readOnly = False;

    shm->attached = true;
    if (shm->XExt.XShmAttach(dpy, &shm->info) == 0)
        goto_error("Failed to attach shared memory for XImage. "
            "Falling back to XPutImage().");

    return 0;
err:
    detach_shm(shm, dpy);
    shm->has_shm_extension = false;
    return 1;
}

static void detach_shm(struct window_x11_shm *shm, Display *dpy)
{
    if (!shm->has_shm_extension) return;

    if (shm->info.shmaddr != (void *)-1) {
        shmdt(shm->info.shmaddr);
        shm->info.shmaddr = (void *) -1;
    }
    if (shm->info.shmid != -1) {
        shmctl(shm->info.shmid, IPC_RMID, NULL);
        shm->info.shmid = -1;
    }
    if (shm->attached) {
        shm->XExt.XShmDetach(dpy, &shm->info);
        shm->attached = false;
    }
}
