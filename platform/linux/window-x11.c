#define _GNU_SOURCE
#include "../window.h"
#include "../thread.h"
#include <core/log.h>
#include <core/util.h>
#include <core/pixel.h>
#include <core/shapes.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <stdatomic.h>
#include <fcntl.h>
#include <unistd.h>
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

static i32 init_acceleration(struct window_x11 *win, enum p_window_flags flags);

static i32 render_init_software(struct x11_render_software_ctx *sw_rctx,
    u16 win_w, u16 win_h, u8 root_depth, xcb_window_t win_handle,
    xcb_connection_t *conn, const struct libxcb *xcb);

static i32 software_init_buffer_malloced(
    struct x11_render_software_malloced_image_buf *buffer_o,
    struct pixel_flat_data *pixbuf_o,
    u16 win_w, u16 win_h, u8 root_depth,
    xcb_connection_t *conn, const struct libxcb *xcb
);
static i32 software_init_buffer_shm(
    struct x11_render_software_shm_buf *buffer_o,
    struct pixel_flat_data *pixbuf_o,
    u16 win_w, u16 win_h, u8 root_depth,
    xcb_connection_t *conn, const struct libxcb *xcb
);
static i32 software_init_buffer_pixmap(
    struct x11_render_software_present_pixmap_buf *buffer_o,
    struct pixel_flat_data *pixbuf_o,
    u16 win_w, u16 win_h, u8 root_depth,
    xcb_connection_t *conn, const struct libxcb *xcb
);

static struct pixel_flat_data * render_present_software(
    struct x11_render_software_ctx *sw_rctx, xcb_window_t win_handle,
    xcb_connection_t *conn, const struct libxcb *xcb
);

static i32 software_present_malloced(
    const struct x11_render_software_malloced_image_buf *buf,
    xcb_connection_t *conn, xcb_window_t win_handle, xcb_gcontext_t window_gc,
    const struct libxcb *xcb
);
static i32 software_present_shm(
    const struct x11_render_software_shm_buf *buf,
    xcb_connection_t *conn, xcb_window_t win_handle, xcb_gcontext_t window_gc,
    const struct libxcb *xcb
);
static i32 software_present_pixmap(
    const struct x11_render_software_present_pixmap_buf *buf,
    xcb_connection_t *conn, xcb_window_t win_handle, xcb_gcontext_t window_gc,
    const struct libxcb *xcb
);

static void render_destroy_software(struct x11_render_software_ctx *sw_rctx,
    xcb_connection_t *conn, const struct libxcb *xcb);

static void software_destroy_buffer_malloced(
    struct x11_render_software_malloced_image_buf *buf,
    xcb_connection_t *conn, const struct libxcb *xcb
);
static void software_destroy_buffer_shm(
    struct x11_render_software_shm_buf *buf,
    xcb_connection_t *conn, const struct libxcb *xcb
);
static void software_destroy_buffer_pixmap(
    struct x11_render_software_present_pixmap_buf *buf,
    xcb_connection_t *conn, const struct libxcb *xcb
);

static i32 render_init_egl(struct x11_render_egl_ctx *egl_rctx,
    const struct libxcb *xcb);
static void render_destroy_egl(struct x11_render_egl_ctx *egl_rctx,
    const struct libxcb *xcb);

static i32 intern_atom(struct window_x11 *win,
    const char *atom_name, xcb_atom_t *o);

static i32 check_xinput2_extension(struct window_x11 *win);
static i32 get_master_input_devices(
    struct window_x11 *win,
    xcb_input_device_id_t *master_mouse_id,
    xcb_input_device_id_t *master_keyboard_id
);

static i32 attach_shm(xcb_shm_segment_info_t *shm_o, u32 w, u32 h,
    xcb_connection_t *conn, const struct libxcb *xcb);
static void detach_shm(xcb_shm_segment_info_t *shm,
    xcb_connection_t *conn, const struct libxcb *xcb);

static void send_dummy_event_to_self(struct window_x11 *win);

#define libxcb_error() (e = win->xcb.xcb_request_check(win->conn, vc), e != NULL)

i32 window_X11_open(struct window_x11 *win, struct p_window_info *info,
    const char *title, const rect_t *area, const u32 flags)
{
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

    vc = win->xcb.xcb_input_xi_select_events_checked(
        win->conn, win->win, 1, &keyboard_mask.head
    );
    if (libxcb_error())
        goto_error("Failed to enable keyboard input handling with Xi2");


    vc = win->xcb.xcb_input_xi_select_events_checked(
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
        win->xcb.shm.has_shm_extension_);
    return 0;

err:
    if (e != NULL) u_nzfree(&e);
    /* window_X11_close() will later be called by p_window_close,
     * so there's no need to do it here */
    return 1;
}

void window_X11_close(struct window_x11 *win)
{
    s_assert(win->exists, "Attempt to double-free window");

    s_log_verbose("Destroying X11 window...");
    if (!win->xcb.failed_) {
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

    if (present_mode == P_WINDOW_PRESENT_VSYNC) {
        s_log_error("VSync is not yet supported in X11 windows");
        return NULL;
    }

    return render_present_software(&win->render.sw,
        win->win, win->conn, &win->xcb);
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

    switch (win->generic_info_p->gpu_acceleration) {
        case P_WINDOW_ACCELERATION_NONE:
            render_destroy_software(&win->render.sw, win->conn, &win->xcb);
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

    win->generic_info_p->gpu_acceleration = P_WINDOW_ACCELERATION_UNSET_;

    switch (new_val) {
    case P_WINDOW_ACCELERATION_UNSET_:
        break;
    case P_WINDOW_ACCELERATION_NONE:
        if (render_init_software(&win->render.sw,
                win->generic_info_p->client_area.w,
                win->generic_info_p->client_area.h,
                win->screen->root_depth, win->win, win->conn, &win->xcb))
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

static i32 render_init_software(struct x11_render_software_ctx *sw_rctx,
    u16 win_w, u16 win_h, u8 root_depth, xcb_window_t win_handle,
    xcb_connection_t *conn, const struct libxcb *xcb)
{
    sw_rctx->initialized_ = true;

    /* Create the window's graphics context (GC) */
    sw_rctx->window_gc = xcb->xcb_generate_id(conn);

#define GCV_MASK XCB_GC_FOREGROUND
    const u32 gcv[] = {
        0 /* black pixel */
    };

    xcb_generic_error_t *e = NULL;
    xcb_void_cookie_t vc = xcb->xcb_create_gc_checked(conn,
        sw_rctx->window_gc, win_handle, GCV_MASK, gcv);
    if (e = xcb->xcb_request_check(conn, vc), e != NULL) {
        u_nzfree(&e);
        s_log_error("Failed to create the graphics context!");
        sw_rctx->window_gc = XCB_NONE;
        return 1;
    }

    for (u32 i = 0; i < 2; i++) {
        struct x11_render_software_buf *const curr_buf = &sw_rctx->buffers[i];
        curr_buf->initialized_ = true;
        curr_buf->type = X11_SWFB_NULL;

        if (software_init_buffer_pixmap(&curr_buf->fb.present_pixmap,
                &curr_buf->pixbuf, win_w, win_h, root_depth, conn, xcb
            ) == 0)
        {
            s_log_verbose("Window framebuffer %d successfully initialized "
                "as X11_SWFB_PRESENT_PIXMAP", i);
            curr_buf->type = X11_SWFB_PRESENT_PIXMAP;
            continue;
        } else {
            s_log_warn("Failed to create window framebuffer %d with "
                "present/pixmap, falling back to shm", i);
            software_destroy_buffer_pixmap(&curr_buf->fb.present_pixmap,
                conn, xcb);
        }

        if (software_init_buffer_shm(&curr_buf->fb.shm,
                &curr_buf->pixbuf, win_w, win_h, root_depth, conn, xcb
            ) == 0)
        {
            s_log_verbose("Window framebuffer %d successfully initialized "
                "as X11_SWFB_SHMSEG", i);
            curr_buf->type = X11_SWFB_SHMSEG;
            continue;
        } else {
            s_log_warn("Failed to create window framebuffer %d with shm, "
                "falling back to manually malloc'd buffers", i);
            software_destroy_buffer_shm(&curr_buf->fb.shm, conn, xcb);
        }

        if (software_init_buffer_malloced(&curr_buf->fb.malloced,
                &curr_buf->pixbuf, win_w, win_h, root_depth, conn, xcb
            ) == 0)
        {
            s_log_verbose("Window framebuffer %d successfully initialized "
                "as X11_SWFB_MALLOCED_IMAGE", i);
            curr_buf->type = X11_SWFB_MALLOCED_IMAGE;
            continue;
        } else {
            s_log_error("Failed to initialize manually allocated buffers");
            return 1;
        }
    }

    sw_rctx->curr_front_buf = &sw_rctx->buffers[0];
    sw_rctx->curr_back_buf = &sw_rctx->buffers[1];

    return 0;
}

static i32 software_init_buffer_malloced(
    struct x11_render_software_malloced_image_buf *buffer_o,
    struct pixel_flat_data *pixbuf_o,
    u16 win_w, u16 win_h, u8 root_depth,
    xcb_connection_t *conn, const struct libxcb *xcb
)
{
    memset(buffer_o, 0, sizeof(struct x11_render_software_malloced_image_buf));

    /* Create the image */
    buffer_o->image = xcb->xcb_image_create_native(conn,
        win_w, win_h, XCB_IMAGE_FORMAT_Z_PIXMAP, root_depth,
        NULL, 0, NULL); /* Let xcb calculate the size */
    if (buffer_o->image == NULL) {
        s_log_error("Failed to create the XCB image");
        return 1;
    }

    /* Fill in the pixel data struct */
    pixbuf_o->w = buffer_o->image->width;
    pixbuf_o->h = buffer_o->image->height;
    pixbuf_o->buf = buffer_o->image->base;

    return 0;
}

static i32 software_init_buffer_shm(
    struct x11_render_software_shm_buf *buffer_o,
    struct pixel_flat_data *pixbuf_o,
    u16 win_w, u16 win_h, u8 root_depth,
    xcb_connection_t *conn, const struct libxcb *xcb
)
{
    memset(buffer_o, 0, sizeof(struct x11_render_software_shm_buf));

    buffer_o->root_depth = root_depth;
    buffer_o->w = win_w;
    buffer_o->h = win_h;

    /* Initialize the shm segment */
    if (!xcb->shm.has_shm_extension_) {
        s_log_error("The shm extension is not initialized");
        return -1;
    }
    if (attach_shm(&buffer_o->shm_info, win_w, win_h, conn, xcb)) {
        s_log_error("Failed to initialize the shared memory segment");
        return 1;
    }

    /* Fill in the pixel data struct */
    pixbuf_o->w = win_w;
    pixbuf_o->h = win_h;
    pixbuf_o->buf = (pixel_t *)buffer_o->shm_info.shmaddr;

    return 0;
}

static i32 software_init_buffer_pixmap(
    struct x11_render_software_present_pixmap_buf *buffer_o,
    struct pixel_flat_data *pixbuf_o,
    u16 win_w, u16 win_h, u8 root_depth,
    xcb_connection_t *conn, const struct libxcb *xcb
)
{
    (void) buffer_o;
    (void) pixbuf_o;
    (void) win_w;
    (void) win_h;
    (void) root_depth;
    (void) conn;
    (void) xcb;
    return -1;
}

static struct pixel_flat_data * render_present_software(
    struct x11_render_software_ctx *sw_rctx, xcb_window_t win_handle,
    xcb_connection_t *conn, const struct libxcb *xcb
)
{
    /* Swap the buffers */
    struct x11_render_software_buf *const tmp = sw_rctx->curr_front_buf;
    sw_rctx->curr_front_buf = sw_rctx->curr_back_buf;
    sw_rctx->curr_back_buf = tmp;

    struct x11_render_software_buf *const curr_buf = sw_rctx->curr_front_buf;
    switch (curr_buf->type) {
    case X11_SWFB_NULL:
        s_log_fatal("Attempt to present an uninitialized buffer");
    case X11_SWFB_MALLOCED_IMAGE:
        software_present_malloced(&curr_buf->fb.malloced,
            conn, win_handle, sw_rctx->window_gc, xcb);
        break;
    case X11_SWFB_SHMSEG:
        software_present_shm(&curr_buf->fb.shm,
            conn, win_handle, sw_rctx->window_gc, xcb);
        break;
    case X11_SWFB_PRESENT_PIXMAP:
        software_present_pixmap(&curr_buf->fb.present_pixmap,
            conn, win_handle, sw_rctx->window_gc, xcb);
        break;
    default:
        s_log_fatal("Invalid buffer type %d", curr_buf->type);
    }

    if (xcb->xcb_flush(conn) <= 0)
        s_log_error("xcb_flush failed!");

    return &sw_rctx->curr_back_buf->pixbuf;
}

static i32 software_present_malloced(
    const struct x11_render_software_malloced_image_buf *buf,
    xcb_connection_t *conn, xcb_window_t win_handle, xcb_gcontext_t window_gc,
    const struct libxcb *xcb
)
{
    (void) xcb->xcb_image_put(conn, win_handle, window_gc, buf->image, 0, 0, 0);
    return 0;
}

static i32 software_present_shm(
    const struct x11_render_software_shm_buf *buf,
    xcb_connection_t *conn, xcb_window_t win_handle, xcb_gcontext_t window_gc,
    const struct libxcb *xcb
)
{
    const u16 TOTAL_SRC_IMAGE_W = buf->w;
    const u16 TOTAL_SRC_IMAGE_H = buf->h;
    const u16 SRC_X = 0, SRC_Y = 0;
    const u16 SRC_W = buf->w;
    const u16 SRC_H = buf->h;

    const i16 DST_X = 0, DST_Y = 0;
    const u8 DST_DEPTH = buf->root_depth;
    const u8 DST_IMAGE_FORMAT = XCB_IMAGE_FORMAT_Z_PIXMAP;

    const u8 SEND_BLIT_COMPLETE_EVENT = true;
    const xcb_shm_seg_t SHMSEG = buf->shm_info.shmseg;
    const u32 SRC_START_OFFSET = 0;

    (void) xcb->shm.xcb_shm_put_image(
        conn, win_handle, window_gc,
        TOTAL_SRC_IMAGE_W, TOTAL_SRC_IMAGE_H, SRC_X, SRC_Y, SRC_W, SRC_H,
        DST_X, DST_Y, DST_DEPTH, DST_IMAGE_FORMAT,
        SEND_BLIT_COMPLETE_EVENT, SHMSEG, SRC_START_OFFSET
    );

    return 0;
}

static i32 software_present_pixmap(
    const struct x11_render_software_present_pixmap_buf *buf,
    xcb_connection_t *conn, xcb_window_t win_handle, xcb_gcontext_t gc,
    const struct libxcb *xcb
)
{
    (void) buf;
    (void) conn;
    (void) win_handle;
    (void) gc;
    (void) xcb;
    return -1;
}

static void render_destroy_software(struct x11_render_software_ctx *sw_rctx,
    xcb_connection_t *conn, const struct libxcb *xcb)
{
    if (sw_rctx == NULL || !sw_rctx->initialized_)
        return;

    for (u32 i = 0; i < 2; i++) {
        if (!sw_rctx->initialized_)
            continue;

        struct x11_render_software_buf *const curr_buf = &sw_rctx->buffers[i];
        switch (curr_buf->type) {
        case X11_SWFB_NULL: break;
        case X11_SWFB_MALLOCED_IMAGE:
            software_destroy_buffer_malloced(&curr_buf->fb.malloced,
                conn, xcb);
            break;
        case X11_SWFB_SHMSEG:
            software_destroy_buffer_shm(&curr_buf->fb.shm, conn, xcb);
            break;
        case X11_SWFB_PRESENT_PIXMAP:
            software_destroy_buffer_pixmap(&curr_buf->fb.present_pixmap,
                conn, xcb);
            break;
        default:
            s_log_fatal("Invalid buffer type %d", curr_buf->type);
        }

        curr_buf->type = X11_SWFB_NULL;
        memset(&curr_buf->pixbuf, 0, sizeof(struct pixel_flat_data));
        curr_buf->initialized_ = false;
    }

    if (sw_rctx->window_gc != XCB_NONE) {
        xcb->xcb_free_gc(conn, sw_rctx->window_gc);
        sw_rctx->window_gc = XCB_NONE;
    }

    sw_rctx->curr_back_buf = sw_rctx->curr_front_buf = NULL;
    sw_rctx->initialized_ = false;
}

static void software_destroy_buffer_malloced(
    struct x11_render_software_malloced_image_buf *buf,
    xcb_connection_t *conn, const struct libxcb *xcb
)
{
    (void) conn;

    if (buf->image != NULL) {
        xcb->xcb_image_destroy(buf->image);
        buf->image = NULL;
    }
}

static void software_destroy_buffer_shm(
    struct x11_render_software_shm_buf *buf,
    xcb_connection_t *conn, const struct libxcb *xcb
)
{
    buf->root_depth = buf->w = buf->h = 0;

    detach_shm(&buf->shm_info, conn, xcb);
}

static void software_destroy_buffer_pixmap(
    struct x11_render_software_present_pixmap_buf *buf,
    xcb_connection_t *conn, const struct libxcb *xcb
)
{
    (void) buf;
    (void) conn;
    (void) xcb;
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
        u_nzfree(&reply);
        return 1;
    }

    *o = reply->atom;
    u_nzfree(&reply);
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

    u_nzfree(&reply);

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

    u_nfree(&reply);
    return found_mouse && found_keyboard ? 0 : 1;
}

static i32 attach_shm(xcb_shm_segment_info_t *shm_o, u32 w, u32 h,
    xcb_connection_t *conn, const struct libxcb *xcb)
{
    if (!xcb->shm.has_shm_extension_) return -1;

    shm_o->shmaddr = (void *)-1;
    shm_o->shmseg = -1;

    /* Create the shmseg */
    shm_o->shmid = shmget(
        IPC_PRIVATE,
        w * h * sizeof(pixel_t),
        IPC_CREAT | 0600
    );
    if ((i32)shm_o->shmid == -1)
        goto_error("Failed to create shared memory: %s", strerror(errno));

    /* Attach (map) the segment to our address space */
    shm_o->shmaddr = shmat(shm_o->shmid, NULL, 0);
    if (shm_o->shmaddr == (void *)-1)
        goto_error("Failed to attach shared memory: %s", strerror(errno));

    shm_o->shmseg = xcb->xcb_generate_id(conn);

    /* Attach the segment to the X server */
    xcb_void_cookie_t vc = xcb->shm.xcb_shm_attach_checked( conn,
        shm_o->shmseg, shm_o->shmid, false);
    xcb_generic_error_t *e = NULL;
    if (e = xcb->xcb_request_check(conn, vc), e != NULL) {
        u_nzfree(&e);
        goto_error("XCB failed to attach the shm segment");
    }

    if (shmctl(shm_o->shmid, IPC_RMID, NULL))
        goto_error("Failed to mark the shm segment to be destroyed "
            "(after the last process detaches): %s", strerror(errno));

    return 0;

err:
    detach_shm(shm_o, conn, xcb);
    return 1;
}

static void detach_shm(xcb_shm_segment_info_t *shm,
    xcb_connection_t *conn, const struct libxcb *xcb)
{
    if (xcb->shm.has_shm_extension_) {
        (void) xcb->shm.xcb_shm_detach(conn, shm->shmseg); /* Never fails */
        if (xcb->xcb_flush(conn) <= 0)
            s_log_error("xcb_flush failed!");
    }

    if (shm->shmaddr != (void *)-1 && shm->shmaddr != NULL) {
        shmdt(shm->shmaddr);
        shm->shmaddr = (void *)-1;
    }
    shm->shmid = -1;
    shm->shmseg = -1;
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
