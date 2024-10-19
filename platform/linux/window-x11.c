#define _GNU_SOURCE
#include "../window.h"
#include <core/log.h>
#include <core/util.h>
#include <core/pixel.h>
#include <core/shapes.h>
#include <core/librtld.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdatomic.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <xcb/xcb.h>
#include <xcb/shm.h>
#include <xcb/xproto.h>
#include <xcb/xinput.h>
#include <xcb/xcb_image.h>
#include <xcb/xcb_icccm.h>
#define P_INTERNAL_GUARD__
#include "window-x11.h"
#undef P_INTERNAL_GUARD__
#define P_INTERNAL_GUARD__
#include "libxcb-rtld.h"
#undef P_INTERNAL_GUARD__
#define P_INTERNAL_GUARD__
#include "mouse-x11.h"
#undef P_INTERNAL_GUARD__
#define P_INTERNAL_GUARD__
#include "window-x11-events.h"
#undef P_INTERNAL_GUARD__

#define MODULE_NAME "window-x11"

static i32 intern_atom(struct window_x11 *win,
    const char *atom_name, xcb_atom_t *o);

static i32 check_xinput2_extension(struct window_x11 *win);
static i32 get_master_input_devices(
    struct window_x11 *win,
    xcb_input_device_id_t *master_mouse_id,
    xcb_input_device_id_t *master_keyboard_id
);

static i32 attach_shm(struct window_x11 *win);
static void detach_shm(struct window_x11 *win);

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

    win->registered_keyboard = NULL;
    win->registered_mouse = NULL;

    if (libxcb_load(&win->xcb)) goto_error("Failed to load libxcb");

    /* Open a connection */
    win->conn = win->xcb.xcb_connect(NULL, NULL);
    if (win->xcb.xcb_connection_has_error(win->conn))
        goto_error("Failed to connect to the X server");

    /* Check if extensions are available */
    if (check_xinput2_extension(win))
        goto_error("The XInput2 extension is not available");

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
        XCB_EVENT_MASK_STRUCTURE_NOTIFY | XCB_EVENT_MASK_EXPOSURE
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

    /* Initialize the XInput2 externsion */
    /* Get the master keyboard and master mouse device IDs */
    if (get_master_input_devices(win,
            &win->master_mouse_id, &win->master_keyboard_id
        )
    ) goto_error("Failed to query master input device IDs");

    /* Credits: https://gist.github.com/LemonBoy/dfe1d7ea428794c65b3d
     * Like come on xcb, WTF?!??!?!! */
    const struct xi2_mask {
        const xcb_input_event_mask_t head;
        const xcb_input_xi_event_mask_t mask;
    } keyboard_mask = {
        .head = {
            .deviceid = win->master_keyboard_id,
            .mask_len = sizeof(keyboard_mask.mask) / sizeof(u32),
        },
        .mask = XCB_INPUT_XI_EVENT_MASK_KEY_PRESS
        | XCB_INPUT_XI_EVENT_MASK_KEY_RELEASE,
    }, mouse_mask = {
        .head = {
            .deviceid = win->master_mouse_id,
            .mask_len = sizeof(mouse_mask.mask) / sizeof(u32),
        },
        .mask = XCB_INPUT_XI_EVENT_MASK_MOTION
        | XCB_INPUT_XI_EVENT_MASK_BUTTON_PRESS
        | XCB_INPUT_XI_EVENT_MASK_BUTTON_RELEASE,
    };

    vc = win->xcb.xcb_input_xi_select_events_checked(
        win->conn, win->win, 1, &keyboard_mask.head
    );
    if (libxcb_error)
        goto_error("Failed to enable keyboard input handling with Xi2");

    vc = win->xcb.xcb_input_xi_select_events_checked(
        win->conn, win->win, 1, &mouse_mask.head
    );
    if (libxcb_error)
        goto_error("Failed to enable mouse input handling with Xi2");

    /* Map the window so that it's visible */
    vc = win->xcb.xcb_map_window_checked(win->conn, win->win);
    if (libxcb_error) goto_error("Failed to map (show) the window");

    /* Allocate the key symbols struct, used by keyboard-x11
     * to map keycodes received from events to keysyms */
    win->key_symbols = win->xcb.xcb_key_symbols_alloc(win->conn);
    if (win->key_symbols == NULL)
        goto_error("Failed to allocate key symbols");

    /* Finally, flush all the commands */
    if (win->xcb.xcb_flush(win->conn) <= 0)
        goto_error("Failed to flush xcb requests");

    /* Create the thread that listens for events */
    atomic_init(&win->listener.running, true);
    if (pthread_create(&win->listener.thread, NULL,
        window_X11_event_listener_fn, win))
    {
        goto_error("Failed to create listener thread: %s", strerror(errno));
    }

    s_log_debug("%s() OK; Screen is %ux%u, use shm: %b", __func__,
        win->screen->width_in_pixels, win->screen->height_in_pixels,
        win->xcb.shm.has_shm_extension_);
    return 0;

err:
    if (e != NULL) u_nzfree(e);
    /* window_X11_close() will later be called by p_window_close,
     * so there's no need to do it here */
    return 1;
}

void window_X11_close(struct window_x11 *win)
{
    if (!win->exists)
        s_log_fatal(MODULE_NAME, __func__, "Attempt to double-free window");

    s_log_debug("Destroying X11 window...");
    if (!win->xcb.failed_) {
        if (win->key_symbols) win->xcb.xcb_key_symbols_free(win->key_symbols);

        if (atomic_load(&win->listener.running)) {
            atomic_store(&win->listener.running, false);

            if (win->win != -1) {
                /* Send an event to our window to interrupt the blocking
                 * `xcb_wait_for_event` call in the listener thread
                 */

                /* XCB will always copy 32 bytes from `ev` */
                char ev_base[32] = { 0 };
                xcb_expose_event_t *ev = (xcb_expose_event_t *)ev_base;

                ev->response_type = XCB_EXPOSE;
                ev->window = win->win;

                (void) win->xcb.xcb_send_event(win->conn, false, win->win,
                        XCB_EVENT_MASK_EXPOSURE, ev_base);
                (void) win->xcb.xcb_flush(win->conn);

                pthread_join(win->listener.thread, NULL);
            } else {
                /* Forcibly terminate the listener if (somehow)
                 * the window does not exist but the thread is running */
                pthread_kill(win->listener.thread, SIGKILL);
            }
        }

        if (win->win != -1) win->xcb.xcb_destroy_window(win->conn, win->win);
        if (win->conn) win->xcb.xcb_disconnect(win->conn);
    }
    libxcb_unload(&win->xcb);

    /* win->exists is also set to false */
    memset(win, 0, sizeof(struct window_x11));
}

void window_X11_render(struct window_x11 *win)
{
    u_check_params(win != NULL && win->fb != NULL &&
        win->fb->buf != NULL && win->fb->w > 0 && win->fb->h > 0);

    if (win->shm_attached) {
#define TOTAL_W win->fb->w
#define TOTAL_H win->fb->h
#define SRC_X 0
#define SRC_Y 0
#define SRC_W win->fb->w
#define SRC_H win->fb->h
#define DST_X 0
#define DST_Y 0
#define SEND_EVENT false
#define OFFSET 0
        (void) win->xcb.shm.xcb_shm_put_image(win->conn, win->win, win->gc,
            TOTAL_W, TOTAL_H, SRC_X, SRC_Y, SRC_W, SRC_H, DST_X, DST_Y,
            win->screen->root_depth, XCB_IMAGE_FORMAT_Z_PIXMAP, SEND_EVENT,
            win->shm_info.shmseg, OFFSET);

    } else if (win->xcb_image != NULL) {
#define LEFT_PAD 0
        (void) win->xcb.xcb_image_put(win->conn, win->win, win->gc,
            win->xcb_image, DST_X, DST_Y, LEFT_PAD);
    }
    (void) win->xcb.xcb_flush(win->conn);
}

void window_X11_bind_fb(struct window_x11 *win, struct pixel_flat_data *fb)
{
    u_check_params(win != NULL && fb != NULL);

    win->fb = fb;

    if (win->xcb.shm.has_shm_extension_) {
        win->xcb_image = NULL;
        if (attach_shm(win))
            goto shm_attach_fail;
    } else {
shm_attach_fail:
        win->xcb_image = win->xcb.xcb_image_create_native(win->conn,
            fb->w, fb->h, XCB_IMAGE_FORMAT_Z_PIXMAP, win->screen->root_depth,
            NULL, fb->w * fb->h * sizeof(pixel_t), (u8 *)fb->buf);

        if (win->xcb_image == NULL)
            s_log_fatal(MODULE_NAME, __func__, "Failed to create XCB image");
    }
}

void window_X11_unbind_fb(struct window_x11 *win)
{
    if (win == NULL || win->fb == NULL || win->fb->buf == NULL)
        return;

    if (win->shm_attached) {
        detach_shm(win);
    } else {
        if (win->fb->buf != NULL) free(win->fb->buf);
        if (win->xcb_image != NULL) {
            win->xcb_image->base = NULL;
            win->xcb_image->data = NULL;

            win->xcb.xcb_image_destroy(win->xcb_image);
            win->xcb_image = NULL;
        }
    }

    memset(win->fb, 0, sizeof(struct pixel_flat_data));
}

i32 window_X11_register_keyboard(struct window_x11 *win,
    struct keyboard_x11 *kb)
{
    u_check_params(win != NULL && kb != NULL);

    if (win->registered_keyboard != NULL) return 1;

    win->registered_keyboard = kb;
    return 0;
}

i32 window_X11_register_mouse(struct window_x11 *win, struct mouse_x11 *mouse)
{
    u_check_params(win != NULL && mouse != NULL);

    if (win->registered_mouse != NULL) return 1;

    win->registered_mouse = mouse;
    return 0;
}

void window_X11_deregister_keyboard(struct window_x11 *win)
{
    u_check_params(win != NULL);
    win->registered_keyboard = NULL;
}

void window_X11_deregister_mouse(struct window_x11 *win)
{
    u_check_params(win != NULL);
    win->registered_mouse = NULL;
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
        u_nzfree(reply);
        return 1;
    }

    *o = reply->atom;
    u_nzfree(reply);
    return 0;
}

static i32 check_xinput2_extension(struct window_x11 *win)
{
    xcb_input_xi_query_version_cookie_t cookie =
        win->xcb.xcb_input_xi_query_version(win->conn, 2, 0);

    xcb_input_xi_query_version_reply_t *reply =
        win->xcb.xcb_input_xi_query_version_reply(win->conn, cookie, NULL);

    if (reply == NULL) return -1;
    else if (reply->major_version < 2) return 1;

    u_nzfree(reply);
    
    return 0;
}

static i32 get_master_input_devices(
    struct window_x11 *win,
    xcb_input_device_id_t *master_mouse_id,
    xcb_input_device_id_t *master_keyboard_id
)
{
    xcb_input_xi_query_device_cookie_t cookie =
        win->xcb.xcb_input_xi_query_device(win->conn, XCB_INPUT_DEVICE_ALL);

    xcb_input_xi_query_device_reply_t *reply =
        win->xcb.xcb_input_xi_query_device_reply(win->conn, cookie, NULL);
    if (reply == NULL)
        return -1;

    xcb_input_xi_device_info_iterator_t iterator =
        win->xcb.xcb_input_xi_query_device_infos_iterator(reply);

    bool found_keyboard = false, found_mouse = false;
    while (iterator.rem > 0 && !(found_keyboard && found_mouse)) {
        xcb_input_xi_device_info_t *device_info = iterator.data;
        
        if (device_info->type == XCB_INPUT_DEVICE_TYPE_MASTER_KEYBOARD) {
            *master_keyboard_id = device_info->deviceid;
            found_keyboard = true;
        } else if (device_info->type == XCB_INPUT_DEVICE_TYPE_MASTER_POINTER) {
            *master_mouse_id = device_info->deviceid;
            found_mouse = true;
        }

        win->xcb.xcb_input_xi_device_info_next(&iterator);
    }

    u_nfree(reply);
    return found_mouse && found_keyboard ? 0 : 1;
}

static i32 attach_shm(struct window_x11 *win)
{
    if (!win->xcb.shm.has_shm_extension_) return -1;

    win->shm_info.shmid = -1;
    win->shm_info.shmaddr = (void *)-1;
    win->shm_info.shmseg = -1;

    win->shm_info.shmid = shmget(
        IPC_PRIVATE,
        win->fb->w * win->fb->h * sizeof(pixel_t),
        IPC_CREAT | 0600
    );
    if (win->shm_info.shmid == -1)
        goto_error("Failed to create shared memory: %s", strerror(errno));

    win->shm_info.shmaddr = shmat(win->shm_info.shmid, NULL, 0);
    if (win->shm_info.shmaddr == (void *)-1)
        goto_error("Failed to attach shared memory: %s", strerror(errno));

    win->shm_info.shmseg = win->xcb.xcb_generate_id(win->conn);
    if (win->shm_info.shmseg == -1)
        goto_error("Failed to generate xcb shm segment ID");

    xcb_void_cookie_t vc = win->xcb.shm.xcb_shm_attach_checked(
        win->conn,
        win->shm_info.shmseg,
        win->shm_info.shmid,
        false
    );
    xcb_generic_error_t *e = NULL;
    if (e = win->xcb.xcb_request_check(win->conn, vc), e != NULL) {
        u_nzfree(e);
        goto_error("XCB failed to attach the shm segment");
    }
    win->shm_attached = true;

    if (win->fb->buf != NULL) free(win->fb->buf);
    win->fb->buf = (pixel_t *)win->shm_info.shmaddr;

    return 0;

err:
    detach_shm(win);
    return 1;
}

static void detach_shm(struct window_x11 *win)
{
    if (!win->shm_attached) return;
    
    (void) win->xcb.shm.xcb_shm_detach(win->conn, win->shm_info.shmseg);

    if (win->shm_info.shmaddr != (void *)-1) {
        shmdt(win->shm_info.shmaddr);
        win->shm_info.shmaddr = (void *)-1;
    }
    win->shm_info.shmid = -1;
    win->shm_info.shmseg = -1;

    win->fb->buf = NULL;
}
