#define _GNU_SOURCE
#include "../window.h"
#include "../thread.h"
#include <core/log.h>
#include <core/util.h>
#include <core/pixel.h>
#include <core/shapes.h>
#include <stdint.h>
#include <string.h>
#include <stdatomic.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <xcb/xcb.h>
#include <xcb/shm.h>
#include <xcb/sync.h>
#include <xcb/xproto.h>
#include <xcb/xinput.h>
#include <xcb/xfixes.h>
#include <xcb/present.h>
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
#define P_INTERNAL_GUARD__
#include "window-x11-present-sw.h"
#undef P_INTERNAL_GUARD__

#define MODULE_NAME "window-x11"

static void init_info(struct p_window_info *info_p,
    const xcb_screen_t *screen, const rect_t *area, const u32 flags);
static xcb_window_t create_window(
    const xcb_screen_t *screen, const struct p_window_info *info,
    xcb_connection_t *conn, const struct libxcb *xcb
);
static i32 set_window_properties(struct window_x11_atoms *atoms,
    const struct p_window_info *info, const char *title, xcb_window_t win,
    xcb_connection_t *conn, const struct libxcb *xcb);
static i32 init_xi2_input(struct window_x11_input *i, xcb_window_t win,
    xcb_connection_t *conn, const struct libxcb *xcb);

static i32 init_acceleration(struct window_x11 *win, enum p_window_flags flags);

static i32 render_init_egl(struct x11_render_egl_ctx *egl_rctx,
    const struct libxcb *xcb);
static void render_destroy_egl(struct x11_render_egl_ctx *egl_rctx,
    const struct libxcb *xcb);

static i32 intern_atom(const char *atom_name, xcb_atom_t *o,
    xcb_connection_t *conn, const struct libxcb *xcb);
static bool query_extension(const char *name,
    xcb_connection_t *conn, const struct libxcb *xcb);

static i32 get_master_input_devices(
    xcb_input_device_id_t *master_mouse_id,
    xcb_input_device_id_t *master_keyboard_id,
    xcb_connection_t *conn, const struct libxcb *xcb
);

static void send_dummy_event_to_self(xcb_window_t win,
    xcb_connection_t *conn, const struct libxcb *xcb);

i32 window_X11_open(struct window_x11 *win, struct p_window_info *info,
    const char *title, const rect_t *area, const u32 flags)
{
    /* Used for error checking */
    xcb_generic_error_t *e = NULL;
    xcb_void_cookie_t vc = { 0 };

    /* Reset the window struct, just in case */
    memset(win, 0, sizeof(struct window_x11));
    win->exists = true;

    /* Check the parameters for compatibility with the 1980s-like
     * X11 protocol constraints */
    if (area->w > UINT16_MAX || area->h > UINT16_MAX
        || area->x < INT16_MIN || area->x > INT16_MAX
        || area->y < INT16_MIN || area->y > INT16_MAX)
    {
        goto_error("The X11 protocol defines window position & dimensions "
            "as 16-bit integers. The requested values would overflow the "
            "16-bit integer limit (<%d, %d> for signed (position) "
            "and <%d, %d> for unsigned (dimensions)).",
            INT16_MIN, INT16_MAX, 0, UINT16_MAX);
    }

    /* Load the requried libraries (`win->xcb`) */
    if (libxcb_load(&win->xcb))
        goto_error("Failed to load libxcb");

    /* Open a connection (`win->conn`) */
    win->conn = win->xcb.xcb_connect(NULL, NULL);
    if (win->xcb.xcb_connection_has_error(win->conn)) {
        /* `window_X11_close` needs to somehow know that the connection
         * is invalid, and the only way to do that without adding
         * a separate flag for it is to destroy the handle here
         * and set the value to `NULL` */
        if (win->conn != NULL) {
            win->xcb.xcb_disconnect(win->conn);
            win->conn = NULL;
        }
        goto_error("Failed to connect to the X server");
    }

    /* Note that this macro should only be used right after calling
     * a `*_checked` xcb function. Otherwise it's useless */
#define libxcb_error() \
    (e = win->xcb.xcb_request_check(win->conn, vc), e != NULL)

    /* Get the server screen configuration (`win->screen`) */

    /* We need to get the screen info early because
     * `init_info` needs it */
    const struct xcb_setup_t *setup = win->xcb.xcb_get_setup(win->conn);
    if (setup == NULL)
        goto_error("Failed to get X server setup");

    /* Get the current screen */
    xcb_screen_iterator_t scr_iter = win->xcb.xcb_setup_roots_iterator(setup);
    win->screen = scr_iter.data;

    /* Initialize the generic window info struct
     * (`win->generic_info_p`) */
    win->generic_info_p = info;
    init_info(win->generic_info_p, win->screen, area, flags);

    /* Create the window (`win->win_handle`) */
    win->win_handle = create_window(win->screen, win->generic_info_p,
        win->conn, &win->xcb);
    if (win->win_handle == XCB_NONE)
        goto err;

    /* Set window properties such as the title, floating state,
     * minimal and maximal size hints, etc
     * (`win->atoms`) */
    if (set_window_properties(&win->atoms, win->generic_info_p, title,
            win->win_handle, win->conn, &win->xcb)
    ) {
        goto_error("Failed to set window properties");
    }

    /* Map the window to make it visible */
    vc = win->xcb.xcb_map_window_checked(win->conn, win->win_handle);
    if (libxcb_error())
        goto_error("Failed to map (show) the window");

    /* Initialize input stuff (`win->input`) */
    if (init_xi2_input(&win->input, win->win_handle, win->conn, &win->xcb))
        goto_error("Failed to initialize input");

    /* Initialize the GPU acceleration based on the flags.
     * This populates `win->render` as well as
     *  the `gpu_acceleration` and `vsync_supported` fields in
     * `win->generic_info_p`. */
    if (init_acceleration(win, flags))
        goto_error("Failed to initialize GPU acceleration");

    /* Flush all the commands that are still queued */
    if (win->xcb.xcb_flush(win->conn) <= 0)
        goto_error("Failed to flush xcb requests");

    /* Create the thread that listens for events
     * (`win->listener`) */
    atomic_init(&win->listener.running, true);
    if (p_mt_thread_create(&win->listener.thread,
        window_X11_event_listener_fn, win))
    {
        goto_error("Failed to create listener thread.");
    }

    s_log_debug("%s() OK; Screen is %ux%u, use shm: %d, use vsync: %d",
        __func__,
        win->screen->width_in_pixels, win->screen->height_in_pixels,
        win->xcb.shm.loaded_, win->generic_info_p->vsync_supported
    );
    return 0;

err:
    if (e != NULL) u_nzfree(&e);
    /* window_X11_close() will later be called by p_window_close,
     * so there's no need to do it here */
    return 1;

#undef libxcb_error
}

void window_X11_close(struct window_x11 *win)
{
    s_assert(win->exists, "Attempt to double-free window");
    win->exists = false;

    s_log_verbose("Destroying X11 window...");
    if (win->xcb.loaded_) {
        /* End the listener thread (`win->listener`) */
        if (atomic_load(&win->listener.running)) {
            atomic_store(&win->listener.running, false);

            if (win->win_handle != XCB_NONE) {
                /* Send an event to our window to interrupt the blocking
                 * `xcb_wait_for_event` call in the listener thread */
                send_dummy_event_to_self(win->win_handle, win->conn, &win->xcb);
                p_mt_thread_wait(&win->listener.thread);
            } else {
                /* Forcibly terminate the listener if (somehow)
                 * the window does not exist but the thread is running */
                p_mt_thread_terminate(&win->listener.thread);
            }
        }

        /* Free acceleration-specific resources
         * (`win->render`) */
        if (win->conn != NULL)
            window_X11_set_acceleration(win, P_WINDOW_ACCELERATION_UNSET_);

        /* Destroy anything related to input
         * (`win->input`) */
        if (win->input.key_symbols)
            win->xcb.xcb_key_symbols_free(win->input.key_symbols);

        p_mt_cond_destroy(&win->input.mouse_deregistration_ack);
        p_mt_cond_destroy(&win->input.keyboard_deregistration_ack);

        /* Reset the atoms (`win->atoms`) */
        memset(&win->atoms, 0, sizeof(struct window_x11_atoms));

        /* Destroy the window itself (`win->win_handle`) */
        if (win->win_handle != XCB_NONE && win->conn != NULL) {
            win->xcb.xcb_destroy_window(win->conn, win->win_handle);
            win->win_handle = XCB_NONE;
        }

        /* Reset the info struct pointer (`win->generic_info_p`) */
        win->generic_info_p = NULL;

        /* Delete the screen info pointer (`win->screen`) */
        win->screen = NULL;

        /* Finally, close the connection and free any xcb resources
         * (`win->conn`) */
        if (win->conn != NULL) {
            win->xcb.xcb_disconnect(win->conn);
            win->conn = NULL;
        }
    }

    /* Unload the xcb libraries (`win->xcb`) */
    libxcb_unload(&win->xcb);
}

struct pixel_flat_data * window_X11_swap_buffers(struct window_x11 *win,
    enum p_window_present_mode present_mode)
{
    u_check_params(win != NULL);

    if (win->generic_info_p->gpu_acceleration != P_WINDOW_ACCELERATION_NONE)
        return NULL;

    if (present_mode == P_WINDOW_PRESENT_VSYNC &&
        !win->generic_info_p->vsync_supported)
    {
        s_log_error("VSync is not supported in this X11 window configuration");
        return NULL;
    }

    return X11_render_present_software(&win->render.sw,
        win->win_handle, win->conn, &win->xcb, present_mode);
}

i32 window_X11_register_keyboard(struct window_x11 *win,
    struct keyboard_x11 *kb)
{
    u_check_params(win != NULL && kb != NULL);

    if (win->input.registered_keyboard != NULL) return 1;

    win->input.registered_keyboard = kb;
    return 0;
}

i32 window_X11_register_mouse(struct window_x11 *win, struct mouse_x11 *mouse)
{
    u_check_params(win != NULL && mouse != NULL);

    if (win->input.registered_mouse != NULL) return 1;

    win->input.registered_mouse = mouse;
    return 0;
}

void window_X11_deregister_keyboard(struct window_x11 *win)
{
    u_check_params(win != NULL);

    if (win->input.registered_keyboard != NULL) {
        /* Notify the event thread that the keyboard is
         * being deregistered and wait for it to acknowledge that */
        p_mt_mutex_t tmp_mutex = p_mt_mutex_create();
        p_mt_mutex_lock(&tmp_mutex);

        atomic_store(&win->input.keyboard_deregistration_notify, true);
        send_dummy_event_to_self(win->win_handle, win->conn, &win->xcb);
        p_mt_cond_wait(win->input.keyboard_deregistration_ack, tmp_mutex);

        win->input.registered_keyboard = NULL;
        atomic_store(&win->input.keyboard_deregistration_notify, false);

        p_mt_mutex_destroy(&tmp_mutex);
    }
}

void window_X11_deregister_mouse(struct window_x11 *win)
{
    u_check_params(win != NULL);
    if (win->input.registered_mouse != NULL) {
        /* Notify the event thread that the mouse is
         * being deregistered and wait for it to acknowledge that */
        p_mt_mutex_t tmp_mutex = p_mt_mutex_create();
        p_mt_mutex_lock(&tmp_mutex);

        atomic_store(&win->input.mouse_deregistration_notify, true);
        send_dummy_event_to_self(win->win_handle, win->conn, &win->xcb);
        p_mt_cond_wait(win->input.mouse_deregistration_ack, tmp_mutex);

        win->input.registered_mouse = NULL;
        atomic_store(&win->input.mouse_deregistration_notify, false);

        p_mt_mutex_destroy(&tmp_mutex);
    }
}

i32 window_X11_set_acceleration(struct window_x11 *win,
    enum p_window_acceleration new_val)
{

    u_check_params(win != NULL && (
        (new_val >= 0 && new_val < P_WINDOW_ACCELERATION_MAX_)
            || new_val == P_WINDOW_ACCELERATION_UNSET_)
    );

    const enum p_window_acceleration old_acceleration =
        win->generic_info_p->gpu_acceleration;
    win->generic_info_p->gpu_acceleration = P_WINDOW_ACCELERATION_UNSET_;

    switch (old_acceleration) {
        case P_WINDOW_ACCELERATION_NONE:
            X11_render_destroy_software(&win->render.sw,
                win->conn, win->win_handle, &win->xcb);
            break;
        case P_WINDOW_ACCELERATION_OPENGL:
            render_destroy_egl(&win->render.egl, &win->xcb);
            break;
        case P_WINDOW_ACCELERATION_UNSET_: /* previously unset, do nothing */
            break;
        case P_WINDOW_ACCELERATION_VULKAN:
        default:
            /* Shouldn't be possible */
            break;
    }

    switch (new_val) {
    case P_WINDOW_ACCELERATION_UNSET_:
        break;
    case P_WINDOW_ACCELERATION_NONE:
        if (X11_render_init_software(&win->render.sw,
                win->generic_info_p->client_area.w,
                win->generic_info_p->client_area.h,
                win->screen->root_depth, win->win_handle, win->conn, &win->xcb,
                &win->generic_info_p->vsync_supported))
        {
            s_log_error("Failed to set up the window for software rendering.");
            return 1;
        }
        break;
    case P_WINDOW_ACCELERATION_OPENGL:
        if (render_init_egl(&win->render.egl, &win->xcb)) {
            s_log_error("Failed to set up the window for EGL rendering.");
            return 1;
        }
        break;
    case P_WINDOW_ACCELERATION_VULKAN:
        s_log_error("Vulkan acceleration not implemented yet.");
        return 1;
    default: /* Technically not possible */
        s_log_error("Unsupported acceleration mode: %i", new_val);
        return 1;
    }

    win->generic_info_p->gpu_acceleration = new_val;
    return 0;
}

i32 X11_check_xinput2_extension(xcb_connection_t *conn,
    const struct libxcb *xcb)
{
    static _Atomic i8 cached_extension_status = ATOMIC_VAR_INIT(-1);

    const i8 cached_extension_status_value =
        atomic_load(&cached_extension_status);

    if (cached_extension_status_value != -1)
        return cached_extension_status_value;

    if (!xcb->xinput.loaded_) {
        s_log_error("The libxcb-xinput client library is not available");
        return -1;
    }

    if (!query_extension(X11_XINPUT_EXT_NAME, conn, xcb)) {
        s_log_error("The XInput extension is not available");
        atomic_store(&cached_extension_status, 1);
        return 1;
    }

    /* Check the extension version */
    xcb_input_xi_query_version_cookie_t cookie =
        xcb->xinput.xcb_input_xi_query_version(conn, 2, 0);
    xcb_input_xi_query_version_reply_t *reply =
        xcb->xinput.xcb_input_xi_query_version_reply(conn, cookie, NULL);

    if (reply == NULL) {
        s_log_error("xcb_input_xi_query_version failed!");
        return -1;
    } else if (reply->major_version < 2) {
        s_log_error("The XInput extension version (%h.%h) is too old - "
            "the required is at least 2.0",
            reply->major_version, reply->minor_version);
        u_nfree(&reply);
        atomic_store(&cached_extension_status, 1);
        return 1;
    }

    u_nfree(&reply);
    atomic_store(&cached_extension_status, 0);
    return 0;
}

i32 X11_check_shm_extension(xcb_connection_t *conn,
    const struct libxcb *xcb)
{
    static _Atomic i8 cached_extension_status = ATOMIC_VAR_INIT(-1);

    const i8 cached_extension_status_value =
        atomic_load(&cached_extension_status);

    if (cached_extension_status_value != -1)
        return cached_extension_status_value;

    if (!xcb->shm.loaded_) {
        s_log_error("The libxcb-shm client library is not available");
        return -1;
    }

    if (!query_extension(X11_SHM_EXT_NAME, conn, xcb)) {
        s_log_error("The MIT-SHM extension is not available");
        atomic_store(&cached_extension_status, 1);
        return 1;
    }

    xcb_shm_query_version_cookie_t cookie =
        xcb->shm.xcb_shm_query_version(conn);
    xcb_shm_query_version_reply_t *reply =
        xcb->shm.xcb_shm_query_version_reply(conn, cookie, NULL);
    if (reply == NULL) {
        s_log_error("xcb_shm_query_version failed!");
        return -1;
    } else if (reply->major_version < 1 ||
            (reply->major_version == 1 && reply->minor_version < 1))
    {
        s_log_error("The X MIT-SHM extension version (%h.%h) is too old - "
            "the required is as least 1.1",
            reply->major_version, reply->minor_version);
        u_nfree(&reply);
        atomic_store(&cached_extension_status, 1);
        return 1;
    }


    u_nfree(&reply);
    atomic_store(&cached_extension_status, 0);
    return 0;
}

i32 X11_check_present_extension(xcb_connection_t *conn,
    const struct libxcb *xcb)
{
    if (!xcb->present.loaded_) {
        s_log_error("The libxcb-present client library is not available!");
        return -1;
    }

    static _Atomic i8 cached_extension_status = ATOMIC_VAR_INIT(-1);

    const i8 cached_extension_status_value =
        atomic_load(&cached_extension_status);
    if (cached_extension_status_value != -1)
        return cached_extension_status_value;

    if (!query_extension(X11_PRESENT_EXT_NAME, conn, xcb)) {
        s_log_error("The X Present extension is not available");
        atomic_store(&cached_extension_status, 1);
        return 1;
    }

    xcb_present_query_version_cookie_t cookie =
        xcb->present.xcb_present_query_version(conn, 1, 1);
    xcb_present_query_version_reply_t *reply =
        xcb->present.xcb_present_query_version_reply(conn, cookie, NULL);

    if (reply == NULL) {
        s_log_error("xcb_present_query_version failed!");
        return -1;
    } else if (reply->major_version < 1 ||
            (reply->major_version == 1 && reply->minor_version < 1))
    {
        s_log_error("The X Present extension version (%h.%h) is too old - "
            "the required is as least 1.1",
            reply->major_version, reply->minor_version);
        u_nfree(&reply);
        atomic_store(&cached_extension_status, 1);
        return 1;
    }

    u_nfree(&reply);
    atomic_store(&cached_extension_status, 0);
    return 0;
}

static void init_info(struct p_window_info *info_p,
    const xcb_screen_t *screen, const rect_t *area, const u32 flags)
{
    /* Handle WINDOW_POS_CENTERED flags */
    i16 x = (i16)area->x, y = (i16)area->y;
    if (flags & P_WINDOW_POS_CENTERED_X)
        x = (screen->width_in_pixels - area->w) / 2;
    if (flags & P_WINDOW_POS_CENTERED_Y)
        y = (screen->height_in_pixels - area->h) / 2;

    info_p->client_area.x = (i32)x;
    info_p->client_area.y = (i32)y;
    info_p->client_area.w = area->w;
    info_p->client_area.h = area->h;

    info_p->display_rect.x = info_p->display_rect.y = 0;
    info_p->display_rect.w = screen->width_in_pixels;
    info_p->display_rect.h = screen->height_in_pixels;
    info_p->display_color_format = BGRX32;

    info_p->gpu_acceleration = P_WINDOW_ACCELERATION_UNSET_;
    info_p->vsync_supported = false; /* To be decided later */
}

static xcb_window_t create_window(
    const xcb_screen_t *screen, const struct p_window_info *info,
    xcb_connection_t *conn, const struct libxcb *xcb
)
{
    /* Generate the window ID */
    xcb_window_t tmp_window_id = xcb->xcb_generate_id(conn);

    /* Create the window */
    const u8 DEPTH = XCB_COPY_FROM_PARENT;
    const xcb_window_t NEW_WINDOW_ID = tmp_window_id;
    const xcb_window_t PARENT_WINDOW_ID = screen->root;
    const i16 X = info->client_area.x, Y = info->client_area.y;
    const u16 W = info->client_area.w, H = info->client_area.h;
    const u16 BORDER_WIDTH = 0;
    const u16 WINDOW_CLASS = XCB_WINDOW_CLASS_INPUT_OUTPUT;
    const xcb_visualid_t VISUAL = screen->root_visual;
    const u32 VALUE_MASK = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
    const u32 VALUE_LIST[] = {
        screen->black_pixel,
        XCB_EVENT_MASK_STRUCTURE_NOTIFY | XCB_EVENT_MASK_EXPOSURE
    };
    xcb_void_cookie_t vc = xcb->xcb_create_window_checked(conn,
        DEPTH, NEW_WINDOW_ID, PARENT_WINDOW_ID, X, Y, W, H,
        BORDER_WIDTH, WINDOW_CLASS, VISUAL, VALUE_MASK, VALUE_LIST);
    xcb_generic_error_t *e = NULL;
    if (e = xcb->xcb_request_check(conn, vc), e != NULL) {
        s_log_error("Failed to create the window!");
        u_nfree(&e);
        return XCB_NONE;
    }

    return tmp_window_id;
}

static i32 set_window_properties(struct window_x11_atoms *atoms,
    const struct p_window_info *info, const char *title, xcb_window_t win,
    xcb_connection_t *conn, const struct libxcb *xcb)
{
    xcb_void_cookie_t vc;
    xcb_generic_error_t *e = NULL;

#define libxcb_error() \
    (e = xcb->xcb_request_check(conn, vc), e != NULL)

    /* Get the UTF8 string atom */
    if (intern_atom("UTF8_STRING", &atoms->UTF8_STRING, conn, xcb))
        goto err;

    /* Set the window title */
    vc = xcb->xcb_change_property_checked(conn, XCB_PROP_MODE_REPLACE,
        win, XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 8, strlen(title), title);
    if (libxcb_error())
        goto_error("Failed to change window name");

    if (intern_atom("_NET_WM_NAME", &atoms->NET_WM_NAME, conn, xcb))
        goto err;
    vc = xcb->xcb_change_property_checked(conn, XCB_PROP_MODE_REPLACE,
        win, atoms->NET_WM_NAME, atoms->UTF8_STRING, 8,
        strlen((char *)title), title
    );
    if (libxcb_error())
        goto_error("Failed to set the _NET_WM_NAME property");

    /* Set the window to floating */
    if (intern_atom("_NET_WM_STATE_ABOVE", &atoms->NET_WM_STATE_ABOVE,
            conn, xcb)
    ) {
        goto err;
    }

    i32 net_wm_state_above_val = 1;
    vc = xcb->xcb_change_property_checked(conn, XCB_PROP_MODE_REPLACE,
        win, atoms->NET_WM_STATE_ABOVE, XCB_ATOM_INTEGER, 32,
        1, &net_wm_state_above_val
    );
    if (libxcb_error())
        goto_error("Failed to set the NET_WM_STATE_ABOVE "
            "(floating window) property");

    /* Set the window minimum and maximum size to the same value
     * to mark the window as non-resizable for the window manager */
    xcb_size_hints_t hints = { 0 };
    xcb->xcb_icccm_size_hints_set_min_size(&hints,
        info->client_area.w, info->client_area.h);
    xcb->xcb_icccm_size_hints_set_max_size(&hints,
        info->client_area.w, info->client_area.h);
    vc = xcb->xcb_icccm_set_wm_normal_hints_checked(conn, win, &hints);
    if (libxcb_error())
        goto_error("Failed to set WM normal hints");

    /* Set the WM_DELETE_WINDOW protocol atom,
     * to enable receiving window close events */
    if (intern_atom("WM_PROTOCOLS", &atoms->WM_PROTOCOLS, conn, xcb))
        goto err;
    if (intern_atom("WM_DELETE_WINDOW", &atoms->WM_DELETE_WINDOW,
            conn, xcb))
        goto err;

    vc = xcb->xcb_change_property_checked(conn, XCB_PROP_MODE_REPLACE,
        win, atoms->WM_PROTOCOLS, XCB_ATOM_ATOM, 32,
        1, &atoms->WM_DELETE_WINDOW
    );
    if (libxcb_error())
        goto_error("Failed to set WM protocols");

    return 0;

err:
    if (e != NULL) {
        u_nfree(&e);
    }
    return 1;

#undef libxcb_error
}

static i32 init_xi2_input(struct window_x11_input *i, xcb_window_t win,
    xcb_connection_t *conn, const struct libxcb *xcb)
{
    xcb_void_cookie_t vc;
    xcb_generic_error_t *e = NULL;

#define libxcb_error() \
    (e = xcb->xcb_request_check(conn, vc), e != NULL)

    /* Initialize the XInput2 externsion */
    if (X11_check_xinput2_extension(conn, xcb))
        goto_error("The XInput2 extension is not available");

    i->xinput_ext_data = xcb->xcb_get_extension_data(conn,
        xcb->xinput.xcb_input_id);
    if (i->xinput_ext_data == NULL)
        goto_error("Couldn't get XInput extension data");

    /* Get the master keyboard and master mouse device IDs */
    if (get_master_input_devices(
            &i->master_mouse_id, &i->master_keyboard_id, conn, xcb
    )) {
        goto_error("Failed to query master input device IDs");
    }

    /* Select the events we want to receive from Xi2 */

    /* Since libxcb has an incorrect struct definition,
     * we have to make our own lol.
     * This is taken directly from the Xi2 protocol specification
     * (https://www.x.org/archive/X11R7.5/doc/inputproto/XI2proto.txt) */
    const struct xi2_event_mask {
        xcb_input_device_id_t deviceid;
        u16 mask_len; /* Length of the mask in 4-byte units */
        xcb_input_xi_event_mask_t mask;
    } keyboard_mask = {
        .deviceid = i->master_keyboard_id,
        .mask_len = 1,
        .mask = XCB_INPUT_XI_EVENT_MASK_KEY_PRESS
            | XCB_INPUT_XI_EVENT_MASK_KEY_RELEASE,
    }, mouse_mask = {
        .deviceid = i->master_mouse_id,
        .mask_len = 1,
        .mask = XCB_INPUT_XI_EVENT_MASK_MOTION
            | XCB_INPUT_XI_EVENT_MASK_BUTTON_PRESS
            | XCB_INPUT_XI_EVENT_MASK_BUTTON_RELEASE,
    };

    vc = xcb->xinput.xcb_input_xi_select_events_checked(conn, win,
        1, (const xcb_input_event_mask_t *)&keyboard_mask);
    if (libxcb_error())
        goto_error("Failed to enable keyboard input handling with Xi2");

    vc = xcb->xinput.xcb_input_xi_select_events_checked(conn, win,
        1, (const xcb_input_event_mask_t *)&mouse_mask);
    if (libxcb_error())
        goto_error("Failed to enable mouse input handling with Xi2");

    /* Allocate the key symbols struct, used by keyboard-x11
     * to map keycodes received from events to keysyms */
    i->key_symbols = xcb->xcb_key_symbols_alloc(conn);
    if (i->key_symbols == NULL)
        goto_error("Failed to allocate key symbols");

    /* Initialize the interfaces to `p_keyboard` and `p_mouse` */
    i->registered_keyboard = NULL;
    i->keyboard_deregistration_ack = p_mt_cond_create();
    atomic_store(&i->keyboard_deregistration_notify, false);

    i->registered_mouse = NULL;
    i->mouse_deregistration_ack = p_mt_cond_create();
    atomic_store(&i->mouse_deregistration_notify, false);

    return 0;

err:
    if (e != NULL) {
        u_nfree(&e);
    }
    return 1;

#undef libxcb_error
}

static i32 init_acceleration(struct window_x11 *win, enum p_window_flags flags)
{
    /* Decide which acceleration modes to try
     * and which are required to succeed */
    const bool try_vulkan = (flags & P_WINDOW_PREFER_ACCELERATED)
        || (flags & P_WINDOW_REQUIRE_ACCELERATED)
        || (flags & P_WINDOW_REQUIRE_VULKAN);
    const bool warn_vulkan = (flags & P_WINDOW_PREFER_ACCELERATED)
        || (flags & P_WINDOW_REQUIRE_ACCELERATED);
    const bool require_vulkan = (flags & P_WINDOW_REQUIRE_VULKAN) || 0;

    const bool try_opengl = (flags & P_WINDOW_PREFER_ACCELERATED)
        || (flags & P_WINDOW_REQUIRE_ACCELERATED)
        || (flags & P_WINDOW_REQUIRE_OPENGL);
    const bool warn_opengl = (flags & P_WINDOW_PREFER_ACCELERATED) || 0;
    const bool require_opengl = (flags & P_WINDOW_REQUIRE_OPENGL)
        || (flags & P_WINDOW_REQUIRE_ACCELERATED);

    const bool try_software = (flags & P_WINDOW_PREFER_ACCELERATED)
        || (flags & P_WINDOW_NO_ACCELERATION);

    win->generic_info_p->gpu_acceleration = P_WINDOW_ACCELERATION_UNSET_;
    if (try_vulkan) {
        if (window_X11_set_acceleration(win, P_WINDOW_ACCELERATION_VULKAN)) {
            if (require_vulkan) {
                s_log_error("Failed to initialize Vulkan.");
                return 1;
            } else if (warn_vulkan && try_opengl) {
                s_log_warn("Failed to initialize Vulkan. "
                    "Falling back to OpenGL.");
            } else if (warn_vulkan && try_software) {
                s_log_warn("Failed to initialize Vulkan. "
                    "Falling back to software rendering.");
            } else if (warn_vulkan) {
                s_log_warn("Failed to initialize Vulkan.");
            }
        } else {
            s_log_verbose("OK initializing Vulkan acceleration.");
            return 0;
        }
    }

    if (try_opengl) {
        if (window_X11_set_acceleration(win, P_WINDOW_ACCELERATION_OPENGL)) {
            if (require_opengl) {
                s_log_error("Failed to initialize OpenGL.");
                return 1;
            } else if (warn_opengl && try_software) {
                s_log_warn("Failed to initialize OpenGL. "
                    "Falling back to software.");
            } else if (warn_opengl) {
                s_log_warn("Failed to initialize OpenGL.");
            }
        } else {
            s_log_verbose("OK initializing OpenGL acceleration.");
            return 0;
        }
    }

    if (try_software) {
        if (window_X11_set_acceleration(win, P_WINDOW_ACCELERATION_NONE)) {
            s_log_error("Failed to initialize software rendering.");
            return 1;
        } else {
            s_log_verbose("OK initializing software rendering.");
            return 0;
        }
    }

    s_log_error("No GPU acceleration mode could be initialized.");
    return 1;
}

static i32 render_init_egl(struct x11_render_egl_ctx *egl_rctx,
    const struct libxcb *xcb)
{
    (void) xcb;
    egl_rctx->initialized_ = true;
    return 0;
}

static void render_destroy_egl(struct x11_render_egl_ctx *egl_rctx,
    const struct libxcb *xcb)
{
    (void) xcb;
    egl_rctx->initialized_ = false;
}

static i32 intern_atom(const char *atom_name, xcb_atom_t *o,
    xcb_connection_t *conn, const struct libxcb *xcb)
{
    xcb_intern_atom_cookie_t cookie = xcb->xcb_intern_atom(conn,
        false, strlen(atom_name), atom_name);

    xcb_generic_error_t *err = NULL;
    xcb_intern_atom_reply_t *reply = xcb->xcb_intern_atom_reply(conn,
        cookie, &err);

    if (err) {
        s_log_error("Failed to intern atom \"%s\"", atom_name);
        u_nfree(&reply);
        return 1;
    }

    *o = reply->atom;
    u_nfree(&reply);
    return 0;
}

static bool query_extension(const char *name,
    xcb_connection_t *conn, const struct libxcb *xcb)
{
    xcb_query_extension_cookie_t cookie = xcb->xcb_query_extension(conn,
        strlen(name), name);
    xcb_query_extension_reply_t *reply =
        xcb->xcb_query_extension_reply(conn, cookie, NULL);
    if (reply == NULL) {
        s_log_error("xcb_query_extension(\"%s\") failed!", name);
        return false;
    }

    const bool ret = reply->present;

    u_nfree(&reply);

    return ret;
}

static i32 get_master_input_devices(
    xcb_input_device_id_t *master_mouse_id,
    xcb_input_device_id_t *master_keyboard_id,
    xcb_connection_t *conn, const struct libxcb *xcb
)
{
    xcb_input_xi_query_device_cookie_t cookie =
        xcb->xinput.xcb_input_xi_query_device(conn, XCB_INPUT_DEVICE_ALL);

    xcb_input_xi_query_device_reply_t *reply =
        xcb->xinput.xcb_input_xi_query_device_reply(conn, cookie, NULL);
    if (reply == NULL)
        return -1;

    xcb_input_xi_device_info_iterator_t iterator =
        xcb->xinput.xcb_input_xi_query_device_infos_iterator(reply);

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

        xcb->xinput.xcb_input_xi_device_info_next(&iterator);
    }

    u_nfree(&reply);
    return found_mouse && found_keyboard ? 0 : 1;
}

static void send_dummy_event_to_self(xcb_window_t win,
    xcb_connection_t *conn, const struct libxcb *xcb)
{
    /* XCB will always copy 32 bytes from `ev` */
    char ev_base[32] = { 0 };
    xcb_expose_event_t *ev = (xcb_expose_event_t *)ev_base;

    ev->response_type = XCB_EXPOSE;
    ev->window = win;

    (void) xcb->xcb_send_event(conn, false, win,
            XCB_EVENT_MASK_EXPOSURE, ev_base);
    if (xcb->xcb_flush(conn) <= 0)
        s_log_error("xcb_flush failed!");
}
