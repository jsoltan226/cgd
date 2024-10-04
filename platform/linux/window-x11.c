#define _GNU_SOURCE
#include "../window.h"
#include "../event.h"
#include "../time.h"
#include <core/log.h>
#include <core/util.h>
#include <core/pixel.h>
#include <core/shapes.h>
#include <core/librtld.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdnoreturn.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <xcb/xcb.h>
#include <xcb/xproto.h>
#include <xcb/xcb_image.h>
#include <xcb/xcb_icccm.h>
#define P_INTERNAL_GUARD__
#include "window-x11.h"
#undef P_INTERNAL_GUARD__
#define P_INTERNAL_GUARD__
#include "libxcb-rtld.h"
#undef P_INTERNAL_GUARD__

#define MODULE_NAME "window-x11"

static i32 intern_atom(struct window_x11 *win,
    const char *atom_name, xcb_atom_t *o);
static void set_icccm_wm_hints(struct window_x11 *win, const rect_t *area);

static i32 init_shm(struct window_x11_shm *shm, xcb_connection_t *conn);

static void * listener_thread_fn(void *arg);
static void handle_event(struct window_x11 *win, xcb_generic_event_t *ev);
static void listener_thread_signal_handler(i32 sig_num);

#define libxcb_error (e = win->xcb.xcb_request_check(win->conn, vc), e != NULL)

i32 window_X11_open(struct window_x11 *win,
    const unsigned char *title, const rect_t *area, const u32 flags)
{
    /* Used for error checking */
    xcb_generic_error_t *e = NULL;
    xcb_void_cookie_t vc = { 0 };

    /* Reset the window struct, just in case */
    memset(win, 0, sizeof(struct window_x11));
    win->exists = true;

    if (libxcb_load(&win->xcb)) goto_error("Failed to load libxcb");

    /* Open a connection */
    win->conn = win->xcb.xcb_connect(NULL, NULL);
    if (win->xcb.xcb_connection_has_error(win->conn))
        goto_error("Failed to connect to the X server");

    /* Get the X server setup */
    win->setup = win->xcb.xcb_get_setup(win->conn);
    if (win->setup == NULL) goto_error("Failed to get X setup");

    /* Get the current screen */
    win->iter = win->xcb.xcb_setup_roots_iterator(win->setup);
    win->screen = win->iter.data;

    /* Generate the window ID */
    win->win = -1;
    win->win = win->xcb.xcb_generate_id(win->conn);
    if (win->win == -1) goto_error("Failed to generate window ID");

    /* Handle WINDOW_POS_CENTERED flags */
    u16 x = area->x, y = area->y;
    if (flags & P_WINDOW_POS_CENTERED_X)
        x = (win->screen->width_in_pixels - area->w) / 2;
    if (flags & P_WINDOW_POS_CENTERED_Y)
        y = (win->screen->height_in_pixels - area->h) / 2;

    /* Create the window */
    u32 value_mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
    u32 value_list[] = {
        win->screen->black_pixel,
        XCB_EVENT_MASK_STRUCTURE_NOTIFY
    };

    vc = win->xcb.xcb_create_window_checked(win->conn, XCB_COPY_FROM_PARENT,
        win->win, win->screen->root,
        x, y, area->w, area->h, 0, XCB_WINDOW_CLASS_INPUT_OUTPUT,
        win->screen->root_visual, value_mask, value_list);
    if (libxcb_error) goto_error("Failed to create the window");

    /* Get the UTF8 string atom */
    if (intern_atom(win, "UTF8_STRING", &win->UTF8_STRING)) goto err;

    /* Set the window title */
    vc = win->xcb.xcb_change_property_checked(win->conn, XCB_PROP_MODE_REPLACE,
        win->win, XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 8,
        strlen((char *)title), title
    );
    if (libxcb_error) goto_error("Failed to change window name");

    if (intern_atom(win, "_NET_WM_NAME", &win->NET_WM_NAME)) goto err;
    vc = win->xcb.xcb_change_property_checked(win->conn, XCB_PROP_MODE_REPLACE,
        win->win, win->NET_WM_NAME, win->UTF8_STRING, 8,
        strlen((char *)title), title
    );
    if (libxcb_error) goto_error("Failed to set the _NET_WM_NAME property");

    /* Set the window to floating */
    if (intern_atom(win,
            "_NET_WM_STATE_ABOVE", &win->NET_WM_STATE_ABOVE
        )
    ) goto err;

    i32 net_wm_state_above_val = 1;
    vc = win->xcb.xcb_change_property_checked(win->conn, XCB_PROP_MODE_REPLACE,
        win->win, win->NET_WM_STATE_ABOVE, XCB_ATOM_INTEGER, 32,
        1, &net_wm_state_above_val
    );
    if (libxcb_error)
        goto_error("Failed to set the NET_WM_STATE_ABOVE property");

    /* Set the window minimum and maximum size */
    xcb_size_hints_t hints = { 0 };
    win->xcb.xcb_icccm_size_hints_set_min_size(&hints, area->w, area->h);
    win->xcb.xcb_icccm_size_hints_set_max_size(&hints, area->w, area->h);
    vc = win->xcb.xcb_icccm_set_wm_normal_hints_checked(win->conn,
            win->win, &hints);
    if (libxcb_error) goto_error("Failed to set WM normal hints");


    /* Set the WM_DELETE_WINDOW protocol atom */
    if (intern_atom(win, "WM_PROTOCOLS", &win->WM_PROTOCOLS))
        goto err;
    if (intern_atom(win, "WM_DELETE_WINDOW", &win->WM_DELETE_WINDOW))
        goto err;

    vc = win->xcb.xcb_change_property_checked(win->conn, XCB_PROP_MODE_REPLACE,
        win->win, win->WM_PROTOCOLS, XCB_ATOM_ATOM, 32,
        1, &win->WM_DELETE_WINDOW
    );
    if (libxcb_error) goto_error("Failed to set WM protocols");

    win->gc = -1;
    win->gc = win->xcb.xcb_generate_id(win->conn);
    if (win->gc == -1) goto_error("Failed to generate Graphics Context ID");

#define GCV_MASK XCB_GC_FOREGROUND
    const u32 gcv[] = {
        win->screen->black_pixel
    };
    vc = win->xcb.xcb_create_gc_checked(win->conn, win->gc, win->win,
        GCV_MASK, gcv);
    if (libxcb_error) goto_error("Failed to create the Graphics Context");

    /* Map the window so that it's visible */
    vc = win->xcb.xcb_map_window_checked(win->conn, win->win);
    if (libxcb_error) goto_error("Failed to map (show) the window");

    /* Finally, flush all the commands */
    if (win->xcb.xcb_flush(win->conn) <= 0)
        goto_error("Failed to flush xcb requests");

    /* Create the thread that listens for events */
    if (pthread_create(&win->listener.thread, NULL, listener_thread_fn, win))
        goto_error("Failed to create listener thread: %s", strerror(errno));
    win->listener.running = true;

    return 0;

err:
    if (e != NULL) free(e);
    /* window_X11_close() will later be called by p_window_close,
     * so there's no need to do it here */
    return 1;
}

void window_X11_close(struct window_x11 *win)
{
    if (!win->exists)
        s_log_fatal(MODULE_NAME, __func__, "Attempt to double-free window");

    if (win->listener.running) {
        win->listener.running = false;
        pthread_kill(win->listener.thread, SIGUSR1);
        pthread_join(win->listener.thread, NULL);
    }
    if (!win->xcb.failed_) {
        if (win->win != -1) win->xcb.xcb_destroy_window(win->conn, win->win);
        if (win->conn) win->xcb.xcb_disconnect(win->conn);
    }
    libxcb_unload(&win->xcb);

    /* win-exists is also set to false */
    memset(win, 0, sizeof(struct window_x11));
}

void window_X11_render(struct window_x11 *win)
{
    u_check_params(win != NULL && win->fb != NULL &&
        win->fb->buf != NULL && win->fb->w > 0 && win->fb->h > 0);

    win->xcb.xcb_image_put(win->conn, win->win, win->gc,
        win->xcb_image, 0, 0, 0);
    win->xcb.xcb_flush(win->conn);
}

void window_X11_bind_fb(struct window_x11 *win, struct pixel_flat_data *fb)
{
    u_check_params(win != NULL && fb != NULL);

    win->fb = fb;
    win->xcb_image = win->xcb.xcb_image_create_native(win->conn, fb->w, fb->h,
        XCB_IMAGE_FORMAT_Z_PIXMAP, win->screen->root_depth, NULL,
        fb->w * fb->h * sizeof(pixel_t), (u8 *)fb->buf);

    if (win->xcb_image == NULL)
        s_log_fatal(MODULE_NAME, __func__, "Failed to create XCB image");
}

void window_X11_unbind_fb(struct window_x11 *win)
{
    if (win == NULL || win->fb == NULL || win->fb->buf == NULL)
        return;

    free(win->fb->buf);
    memset(win->fb, 0, sizeof(struct pixel_flat_data));

    if (win->xcb_image != NULL) {
        win->xcb_image->base = NULL;
        win->xcb_image->data = NULL;

        win->xcb.xcb_image_destroy(win->xcb_image);
        win->xcb_image = NULL;
    }
}

static i32 intern_atom(struct window_x11 *win,
    const char *atom_name, xcb_atom_t *o)
{
    xcb_intern_atom_cookie_t cookie = win->xcb.xcb_intern_atom(win->conn,
        false, strlen(atom_name), atom_name);

    xcb_generic_error_t *err = NULL;
    xcb_intern_atom_reply_t *reply = win->xcb.xcb_intern_atom_reply(win->conn,
        cookie, &err);

    if (err) {
        s_log_error("Failed to intern atom \"%s\"", atom_name);
        free(reply);
        return 1;
    }

    *o = reply->atom;
    free(reply);
    return 0;
}

static void set_icccm_wm_hints(struct window_x11 *win, const rect_t *area)
{
    xcb_void_cookie_t vc;
    xcb_generic_error_t *e = NULL;

}

static void * listener_thread_fn(void *arg)
{
    struct window_x11 *win = (struct window_x11 *)arg;

    /* Make the thread ignore all signals except SIGUSR1 */
    sigset_t sig_set;
    sigfillset(&sig_set);
    sigdelset(&sig_set, SIGUSR1);
    if (pthread_sigmask(SIG_BLOCK, &sig_set, NULL)) {
        s_log_fatal(MODULE_NAME, __func__,
            "Failed to set signal mask: %s", strerror(errno));
    }

    /* Set up the signal handler */
    struct sigaction sa = { 0 };
    sa.sa_handler = listener_thread_signal_handler;
    /* Block all signals while our handler is running */
    sigfillset(&sa.sa_mask);

    if (sigaction(SIGUSR1, &sa, NULL)) {
        s_log_fatal(MODULE_NAME, __func__,
            "Failed to set SIGUSR1 handler: %s", strerror(errno));
    }

    xcb_generic_event_t *ev = NULL;
    while (win->listener.running) {
        p_time_usleep(10000);
        while (ev = win->xcb.xcb_poll_for_event(win->conn), ev)
            handle_event(win, ev);
    }

    /* Reset the signal handler */
    sa.sa_handler = SIG_DFL;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGUSR1, &sa, NULL);

    pthread_exit(NULL);
}

static void handle_event(struct window_x11 *win, xcb_generic_event_t *ev)
{
    switch (ev->response_type & ~0x80) {
    case XCB_CLIENT_MESSAGE: {
        xcb_client_message_event_t *cm = (xcb_client_message_event_t *)ev;
        if (cm->data.data32[0] == win->WM_DELETE_WINDOW)
            p_event_send(&(struct p_event) { .type = P_EVENT_QUIT });

        break;
    }
    case XCB_DESTROY_NOTIFY:
        p_event_send(&(struct p_event) { .type = P_EVENT_QUIT });
        break;
    default:
        break;
    }

    free(ev);
}

static void listener_thread_signal_handler(i32 sig_num)
{
}
