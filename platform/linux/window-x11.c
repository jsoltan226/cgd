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

static i32 init_acceleration(struct window_x11 *win, enum p_window_flags flags);

static i32 render_init_egl(struct x11_render_egl_ctx *egl_rctx,
    const struct libxcb *xcb);
static void render_destroy_egl(struct x11_render_egl_ctx *egl_rctx,
    const struct libxcb *xcb);

static i32 intern_atom(struct window_x11 *win,
    const char *atom_name, xcb_atom_t *o);
static bool query_extension(const char *name,
    xcb_connection_t *conn, const struct libxcb *xcb);

static i32 get_master_input_devices(
    struct window_x11 *win,
    xcb_input_device_id_t *master_mouse_id,
    xcb_input_device_id_t *master_keyboard_id
);

static void send_dummy_event_to_self(struct window_x11 *win);

i32 window_X11_open(struct window_x11 *win, struct p_window_info *info,
    const char *title, const rect_t *area, const u32 flags)
{
#define libxcb_error() (e = win->xcb.xcb_request_check(win->conn, vc), e != NULL)

    /* Used for error checking */
    xcb_generic_error_t *e = NULL;
    xcb_void_cookie_t vc = { 0 };

    /* Reset the window struct, just in case */
    memset(win, 0, sizeof(struct window_x11));
    win->exists = true;

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

    win->generic_info_p = info;

    win->registered_keyboard = NULL;
    win->keyboard_deregistration_ack = p_mt_cond_create();
    atomic_store(&win->keyboard_deregistration_notify, false);

    win->registered_mouse = NULL;
    win->mouse_deregistration_ack = p_mt_cond_create();
    atomic_store(&win->mouse_deregistration_notify, false);

    if (libxcb_load(&win->xcb)) goto_error("Failed to load libxcb");

    /* Open a connection */
    win->conn = win->xcb.xcb_connect(NULL, NULL);
    if (win->xcb.xcb_connection_has_error(win->conn))
        goto_error("Failed to connect to the X server");

    /* Initialize Xinput v2 */
    if (X11_check_xinput2_extension(win->conn, &win->xcb))
        goto_error("The XInput2 extension is not available");

    win->xinput_ext_data = win->xcb.xcb_get_extension_data(win->conn,
        win->xcb.xinput.xcb_input_id);
    if (win->xinput_ext_data == NULL)
        goto_error("Couldn't get XInput extension data");

    /* Get the X server setup */
    win->setup = win->xcb.xcb_get_setup(win->conn);
    if (win->setup == NULL) goto_error("Failed to get X setup");

    /* Get the current screen */
    win->iter = win->xcb.xcb_setup_roots_iterator(win->setup);
    win->screen = win->iter.data;

    /* Generate the window ID */
    win->win = win->xcb.xcb_generate_id(win->conn);

    /* Handle WINDOW_POS_CENTERED flags */
    i16 x = (i16)area->x, y = (i16)area->y;
    if (flags & P_WINDOW_POS_CENTERED_X)
        x = (win->screen->width_in_pixels - area->w) / 2;
    if (flags & P_WINDOW_POS_CENTERED_Y)
        y = (win->screen->height_in_pixels - area->h) / 2;

    /* Initialize the generic window parameters */
    info->client_area.x = (i32)x;
    info->client_area.y = (i32)y;
    info->client_area.w = area->w;
    info->client_area.h = area->h;

    info->display_rect.x = info->display_rect.y = 0;
    info->display_rect.w = win->screen->width_in_pixels;
    info->display_rect.h = win->screen->height_in_pixels;
    info->display_color_format = BGRX32;

    info->gpu_acceleration = P_WINDOW_ACCELERATION_UNSET_;
    info->vsync_supported = false;

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
    if (libxcb_error()) goto_error("Failed to create the window");
    win->win_created = true;

    /* Get the UTF8 string atom */
    if (intern_atom(win, "UTF8_STRING", &win->UTF8_STRING)) goto err;

    /* Set the window title */
    vc = win->xcb.xcb_change_property_checked(win->conn, XCB_PROP_MODE_REPLACE,
        win->win, XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 8,
        strlen((char *)title), title
    );
    if (libxcb_error()) goto_error("Failed to change window name");

    if (intern_atom(win, "_NET_WM_NAME", &win->NET_WM_NAME)) goto err;
    vc = win->xcb.xcb_change_property_checked(win->conn, XCB_PROP_MODE_REPLACE,
        win->win, win->NET_WM_NAME, win->UTF8_STRING, 8,
        strlen((char *)title), title
    );
    if (libxcb_error()) goto_error("Failed to set the _NET_WM_NAME property");

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
    if (libxcb_error())
        goto_error("Failed to set the NET_WM_STATE_ABOVE property");

    /* Set the window minimum and maximum size */
    xcb_size_hints_t hints = { 0 };
    win->xcb.xcb_icccm_size_hints_set_min_size(&hints, area->w, area->h);
    win->xcb.xcb_icccm_size_hints_set_max_size(&hints, area->w, area->h);
    vc = win->xcb.xcb_icccm_set_wm_normal_hints_checked(win->conn,
            win->win, &hints);
    if (libxcb_error()) goto_error("Failed to set WM normal hints");


    /* Set the WM_DELETE_WINDOW protocol atom */
    if (intern_atom(win, "WM_PROTOCOLS", &win->WM_PROTOCOLS))
        goto err;
    if (intern_atom(win, "WM_DELETE_WINDOW", &win->WM_DELETE_WINDOW))
        goto err;

    vc = win->xcb.xcb_change_property_checked(win->conn, XCB_PROP_MODE_REPLACE,
        win->win, win->WM_PROTOCOLS, XCB_ATOM_ATOM, 32,
        1, &win->WM_DELETE_WINDOW
    );
    if (libxcb_error()) goto_error("Failed to set WM protocols");

    /* Initialize the graphics context */

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

    vc = win->xcb.xinput.xcb_input_xi_select_events_checked(
        win->conn, win->win, 1, &keyboard_mask.head
    );
    if (libxcb_error())
        goto_error("Failed to enable keyboard input handling with Xi2");


    vc = win->xcb.xinput.xcb_input_xi_select_events_checked(
        win->conn, win->win, 1, &mouse_mask.head
    );
    if (libxcb_error())
        goto_error("Failed to enable mouse input handling with Xi2");

    /* Allocate the key symbols struct, used by keyboard-x11
     * to map keycodes received from events to keysyms */
    win->key_symbols = win->xcb.xcb_key_symbols_alloc(win->conn);
    if (win->key_symbols == NULL)
        goto_error("Failed to allocate key symbols");

    /* Initialize the GPU acceleration based on the flags */
    if (init_acceleration(win, flags))
        goto_error("Failed to initialize GPU acceleration");

    /* Map the window so that it's visible */
    vc = win->xcb.xcb_map_window_checked(win->conn, win->win);
    if (libxcb_error()) goto_error("Failed to map (show) the window");

    /* Finally, flush all the commands */
    if (win->xcb.xcb_flush(win->conn) <= 0)
        goto_error("Failed to flush xcb requests");

    /* Create the thread that listens for events */
    atomic_init(&win->listener.running, true);
    if (p_mt_thread_create(&win->listener.thread,
        window_X11_event_listener_fn, win))
    {
        goto_error("Failed to create listener thread.");
    }

    s_log_debug("%s() OK; Screen is %ux%u, use shm: %i", __func__,
        win->screen->width_in_pixels, win->screen->height_in_pixels,
        win->xcb.shm.loaded_);
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

    s_log_verbose("Destroying X11 window...");
    if (win->xcb.loaded_) {
        /* Free acceleration-specific resources */
        if (win->conn)
            window_X11_set_acceleration(win, P_WINDOW_ACCELERATION_UNSET_);

        if (win->key_symbols) win->xcb.xcb_key_symbols_free(win->key_symbols);

        if (atomic_load(&win->listener.running)) {
            atomic_store(&win->listener.running, false);

            if (win->win_created) {
                /* Send an event to our window to interrupt the blocking
                 * `xcb_wait_for_event` call in the listener thread */
                send_dummy_event_to_self(win);
                p_mt_thread_wait(&win->listener.thread);
            } else {
                /* Forcibly terminate the listener if (somehow)
                 * the window does not exist but the thread is running */
                p_mt_thread_terminate(&win->listener.thread);
            }
        }

        if (win->win_created) win->xcb.xcb_destroy_window(win->conn, win->win);
        if (win->conn) win->xcb.xcb_disconnect(win->conn);
    }
    libxcb_unload(&win->xcb);

    p_mt_cond_destroy(&win->mouse_deregistration_ack);
    p_mt_cond_destroy(&win->keyboard_deregistration_ack);

    /* win->exists is also set to false */
    memset(win, 0, sizeof(struct window_x11));
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
        win->win, win->conn, &win->xcb, present_mode);
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

    if (win->registered_keyboard != NULL) {
        /* Notify the event thread that the keyboard is
         * being deregistered and wait for it to acknowledge that */
        p_mt_mutex_t tmp_mutex = p_mt_mutex_create();
        p_mt_mutex_lock(&tmp_mutex);

        atomic_store(&win->keyboard_deregistration_notify, true);
        send_dummy_event_to_self(win);
        p_mt_cond_wait(win->keyboard_deregistration_ack, tmp_mutex);

        win->registered_keyboard = NULL;
        atomic_store(&win->keyboard_deregistration_notify, false);

        p_mt_mutex_destroy(&tmp_mutex);
    }
}

void window_X11_deregister_mouse(struct window_x11 *win)
{
    u_check_params(win != NULL);
    if (win->registered_mouse != NULL) {
        /* Notify the event thread that the mouse is
         * being deregistered and wait for it to acknowledge that */
        p_mt_mutex_t tmp_mutex = p_mt_mutex_create();
        p_mt_mutex_lock(&tmp_mutex);

        atomic_store(&win->mouse_deregistration_notify, true);
        send_dummy_event_to_self(win);
        p_mt_cond_wait(win->mouse_deregistration_ack, tmp_mutex);

        win->registered_mouse = NULL;
        atomic_store(&win->mouse_deregistration_notify, false);

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
                win->conn, win->win, &win->xcb);
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
                win->screen->root_depth, win->win, win->conn, &win->xcb,
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
    struct window_x11 *win,
    xcb_input_device_id_t *master_mouse_id,
    xcb_input_device_id_t *master_keyboard_id
)
{
    xcb_input_xi_query_device_cookie_t cookie =
        win->xcb.xinput.xcb_input_xi_query_device(win->conn,
            XCB_INPUT_DEVICE_ALL);

    xcb_input_xi_query_device_reply_t *reply =
        win->xcb.xinput.xcb_input_xi_query_device_reply(win->conn,
            cookie, NULL);
    if (reply == NULL)
        return -1;

    xcb_input_xi_device_info_iterator_t iterator =
        win->xcb.xinput.xcb_input_xi_query_device_infos_iterator(reply);

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

        win->xcb.xinput.xcb_input_xi_device_info_next(&iterator);
    }

    u_nfree(&reply);
    return found_mouse && found_keyboard ? 0 : 1;
}

static void send_dummy_event_to_self(struct window_x11 *win)
{
    /* XCB will always copy 32 bytes from `ev` */
    char ev_base[32] = { 0 };
    xcb_expose_event_t *ev = (xcb_expose_event_t *)ev_base;

    ev->response_type = XCB_EXPOSE;
    ev->window = win->win;

    (void) win->xcb.xcb_send_event(win->conn, false, win->win,
            XCB_EVENT_MASK_EXPOSURE, ev_base);
    if (win->xcb.xcb_flush(win->conn) <= 0)
        s_log_error("xcb_flush failed!");
}
