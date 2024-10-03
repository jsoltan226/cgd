#define _GNU_SOURCE
#include "../event.h"
#include "../time.h"
#include <core/log.h>
#include <core/util.h>
#include <core/pixel.h>
#include <core/shapes.h>
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
#include <xcb/xcb_atom.h>
#define P_INTERNAL_GUARD__
#include "window-x11.h"
#undef P_INTERNAL_GUARD__

#define MODULE_NAME "window-x11"

static void * listener_thread_fn(void *arg);
static void handle_event(struct window_x11 *win, xcb_generic_event_t *ev);
static void listener_thread_signal_handler(i32 sig_num);

i32 window_X11_open(struct window_x11 *win,
    const unsigned char *title, const rect_t *area, const u32 flags)
{
    /* Used for error checking */
    xcb_generic_error_t *e = NULL;
    xcb_void_cookie_t vc = { 0 };

    xcb_intern_atom_reply_t *wm_protocols_reply = NULL;
    xcb_intern_atom_reply_t *wm_delete_window_reply = NULL;

    /* Reset the window struct, just in case */
    memset(win, 0, sizeof(struct window_x11));

    /* Open a connection */
    win->conn = xcb_connect(NULL, NULL);
    if (xcb_connection_has_error(win->conn))
        goto_error("Failed to connect to the X server");

    /* Get the X server setup */
    win->setup = xcb_get_setup(win->conn);
    if (win->setup == NULL)
        goto_error("Failed to get X setup");

    /* Get the current screen */
    win->iter = xcb_setup_roots_iterator(win->setup);
    win->screen = win->iter.data;

    /* Generate the window ID */
    win->win = -1;
    win->win = xcb_generate_id(win->conn);
    if (win->win == -1)
        goto_error("Failed to generate window ID");

    /* Create the window */
    u32 value_mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
    u32 value_list[] = {
        win->screen->black_pixel,
        XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_STRUCTURE_NOTIFY
    };

    vc = xcb_create_window(win->conn, XCB_COPY_FROM_PARENT, win->win, win->screen->root,
        area->x, area->y, area->w, area->h, 0, XCB_WINDOW_CLASS_INPUT_OUTPUT,
        win->screen->root_visual, value_mask, value_list);

    if (e = xcb_request_check(win->conn, vc), e != NULL)
        goto_error("Failed to create the window");

    /* Set the window title */
    vc = xcb_change_property(win->conn, XCB_PROP_MODE_REPLACE, win->win,
        XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 8, strlen((char *)title), title);
    if (e = xcb_request_check(win->conn, vc), e != NULL)
        goto_error("Failed to change window name");

    /* Set the WM_DELETE_WINDOW protocol atom */
    xcb_intern_atom_cookie_t wm_protocols_cookie = xcb_intern_atom(
        win->conn, 0, u_strlen("WM_PROTOCOLS"), "WM_PROTOCOLS"
    );
    xcb_intern_atom_cookie_t wm_delete_window_cookie = xcb_intern_atom(
        win->conn, 0, u_strlen("WM_DELETE_WINDOW"), "WM_DELETE_WINDOW"
    );

    wm_protocols_reply = xcb_intern_atom_reply(
        win->conn, wm_protocols_cookie, NULL
    );
    if (wm_protocols_reply == NULL)
        goto_error("Failed to intern the WM_PROTOCOLS atom");

    wm_delete_window_reply = xcb_intern_atom_reply(
        win->conn, wm_delete_window_cookie, NULL
    );
    if (wm_delete_window_reply == NULL)
        goto_error("Failed to intern the WM_DELETE_WINDOW atom");

    win->WM_PROTOCOLS = wm_protocols_reply->atom;
    win->WM_DELETE_WINDOW = wm_delete_window_reply->atom;

    free(wm_protocols_reply);
    free(wm_delete_window_reply);
    wm_protocols_reply = NULL;
    wm_delete_window_reply = NULL;

    vc = xcb_change_property(
        win->conn,
        XCB_PROP_MODE_REPLACE,
        win->win,
        win->WM_PROTOCOLS,
        XCB_ATOM_ATOM,
        32,
        1,
        &win->WM_DELETE_WINDOW
    );
    if (e = xcb_request_check(win->conn, vc), e != NULL)
        goto_error("Failed to set WM protocols");

    /* Map the window so that it's visible */
    vc = xcb_map_window(win->conn, win->win);
    if (e = xcb_request_check(win->conn, vc), e != NULL)
        goto_error("Failed to map (show) the window");

    /* Finally, flush all the commands */
    if (xcb_flush(win->conn) <= 0)
        goto_error("Failed to flush xcb requests");

    /* Create the thread that listens for events */
    if (pthread_create(&win->listener.thread, NULL, listener_thread_fn, win))
        goto_error("Failed to create listener thread: %s", strerror(errno));
    win->listener.running = true;

    return 0;

err:
    if (e != NULL) free(e);
    if (wm_delete_window_reply != NULL) free(wm_delete_window_reply);
    if (wm_protocols_reply != NULL) free(wm_protocols_reply);
    /* window_X11_close() will later be called by p_window_close,
     * so there's no need to do it here */
    return 1;
}

void window_X11_close(struct window_x11 *win)
{
    if (win->listener.running) {
        win->listener.running = false;
        pthread_kill(win->listener.thread, SIGUSR1);
        pthread_join(win->listener.thread, NULL);
    }
    if (win->win != -1) xcb_destroy_window(win->conn, win->win);
    if (win->conn) xcb_disconnect(win->conn);
    memset(win, 0, sizeof(struct window_x11));
}

void window_X11_render(struct window_x11 *win)
{
}

void window_X11_bind_fb(struct window_x11 *win, struct pixel_flat_data *fb)
{
    u_check_params(win != NULL && fb != NULL);

    win->fb = fb;
}

void window_X11_unbind_fb(struct window_x11 *win)
{
    if (win == NULL || win->fb == NULL || win->fb->buf == NULL)
        return;

    free(win->fb->buf);
    memset(win->fb, 0, sizeof(struct pixel_flat_data));
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
        while (ev = xcb_poll_for_event(win->conn), ev)
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
