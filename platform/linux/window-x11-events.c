#define _GNU_SOURCE
#include "../event.h"
#include "../mouse.h"
#include "../window.h"
#include <core/log.h>
#include <core/int.h>
#include <core/math.h>
#include <core/util.h>
#include <core/spinlock.h>
#include <stdatomic.h>
#include <pthread.h>
#include <xcb/xcb.h>
#include <xcb/shm.h>
#include <xcb/xproto.h>
#include <xcb/xcbext.h>
#include <xcb/xinput.h>
#include <xcb/present.h>
#include <xcb/xcb_event.h>
#define P_INTERNAL_GUARD__
#include "window-x11-events.h"
#undef P_INTERNAL_GUARD__
#define P_INTERNAL_GUARD__
#include "window-x11.h"
#undef P_INTERNAL_GUARD__
#define P_INTERNAL_GUARD__
#include "window-x11-extensions.h"
#undef P_INTERNAL_GUARD__
#define P_INTERNAL_GUARD__
#include "window-x11-present-sw.h"
#undef P_INTERNAL_GUARD__
#define P_INTERNAL_GUARD__
#include "libxcb-rtld.h"
#undef P_INTERNAL_GUARD__
#define P_INTERNAL_GUARD__
#include "mouse-x11.h"
#undef P_INTERNAL_GUARD__
#define P_INTERNAL_GUARD__
#include "keyboard-x11.h"
#undef P_INTERNAL_GUARD__

#define MODULE_NAME "window-x11-events"

static void handle_event(struct window_x11 *win, xcb_generic_event_t *ev);
static void handle_ge_event(struct window_x11 *win,
    xcb_ge_generic_event_t *ev);
static void handle_xi2_event(struct window_x11 *win,
    xcb_ge_generic_event_t *ev);
static void handle_present_event(struct window_x11 *win,
    xcb_ge_generic_event_t *ev);
static void handle_shm_completion_event(struct window_x11 *win,
    xcb_generic_event_t *ev);
static void handle_shm_error(struct window_x11 *win, xcb_generic_error_t *e);

void * window_X11_event_listener_fn(void *arg)
{
    struct window_x11 *win = (struct window_x11 *)arg;

    while (atomic_load(&win->listener.running)) {
        xcb_generic_event_t *ev = win->xcb.xcb_wait_for_event(win->conn);
        if (ev == NULL)
            s_log_fatal("Connection to X server broken - I/O Error");

        handle_event(win, ev);
        free(ev);
    }

    pthread_exit(NULL);
}

static void handle_event(struct window_x11 *win, xcb_generic_event_t *ev)
{
    const enum p_window_acceleration acceleration =
        win->generic_info_p->gpu_acceleration;
    const struct x11_render_shared_buffer_data *const shared_buf_data =
        acceleration == P_WINDOW_ACCELERATION_NONE ?
            &win->render.sw.shared_buf_data :
            NULL;

    s_log_trace("Event %u", XCB_EVENT_RESPONSE_TYPE(ev));

    struct x11_extension shm_extension;
    X11_extension_get_data(&win->ext_store, X11_EXT_SHM, &shm_extension);
    if (acceleration == P_WINDOW_ACCELERATION_NONE &&
        shared_buf_data->shm.initialized_ &&
        XCB_EVENT_RESPONSE_TYPE(ev) == shm_extension.first_event)
    {
        handle_shm_completion_event(win, ev);
        goto end;
    }

    switch (XCB_EVENT_RESPONSE_TYPE(ev)) {
    case XCB_CLIENT_MESSAGE: {
        xcb_client_message_event_t *cm = (xcb_client_message_event_t *)ev;
        if (cm->data.data32[0] == win->atoms.WM_DELETE_WINDOW)
            p_event_send(&(struct p_event) { .type = P_EVENT_QUIT });

        break;
    }
    case XCB_DESTROY_NOTIFY:
        p_event_send(&(struct p_event) { .type = P_EVENT_QUIT });
        break;
    case XCB_GE_GENERIC:
        handle_ge_event(win, (xcb_ge_generic_event_t *)ev);
        break;
    case XCB_EXPOSE:
        break;
    case 0:
        if (acceleration == P_WINDOW_ACCELERATION_NONE &&
            shared_buf_data->shm.initialized_ &&
            ((xcb_generic_error_t *)ev)->major_code ==
                shm_extension.major_opcode)
        {
            handle_shm_error(win, (xcb_generic_error_t *)ev);
        } else {
            s_log_error("X error %u (extension: %u, request: %u)",
                ((xcb_generic_error_t *)ev)->error_code,
                ((xcb_generic_error_t *)ev)->major_code,
                ((xcb_generic_error_t *)ev)->minor_code
            );
        }
        break;
    default:
        break;
    }

end:;

    /* If we get a notification that the keyboard and/or mouse
     * are being deregistered, acknowledge it */
    for (u32 i = 0; i < X11_INPUT_REG_MAX_; i++) {
        struct x11_registered_input_obj *const obj =
            &win->input.registered_input_objs[i];
        if (!atomic_load(&obj->active_))
            continue;

        s_assert(pthread_mutex_lock(&obj->mutex) == 0,
            "impossible outcome");
        {
            if (obj->dereg_notify) {
                s_log_trace("Deregistration of object ID %d", i);
                obj->dereg_notify = false;
                (void) pthread_cond_broadcast(&obj->dereg_ack_cond);
            }
        }
        s_assert(pthread_mutex_unlock(&obj->mutex) == 0,
            "impossible outcome");
    }
}

static void handle_ge_event(struct window_x11 *win,
    xcb_ge_generic_event_t *ge_ev)
{
    const enum p_window_acceleration acceleration =
        win->generic_info_p->gpu_acceleration;
    const struct x11_render_shared_buffer_data *const shared_buf_data =
        acceleration == P_WINDOW_ACCELERATION_NONE ?
            &win->render.sw.shared_buf_data :
            NULL;

    const enum x11_extension_name ext =
        X11_extension_get_name_by_opcode(&win->ext_store, ge_ev->extension);

    switch (ext) {
    case X11_EXT_XINPUT:
        handle_xi2_event(win, ge_ev);
        break;
    case X11_EXT_PRESENT:
        if (acceleration != P_WINDOW_ACCELERATION_NONE
            || !atomic_load(&shared_buf_data->present.initialized_))
        {
            s_log_debug("Present event received while "
                "no X11_SWFB_PRESENT_PIXMAPs are in use; ignoring");
            break;
        }
        handle_present_event(win, ge_ev);
        break;
    default: case X11_EXT_SHM: case X11_EXT_NULL_:
        s_log_error("Unhandled extension %u event %u",
            ge_ev->extension, ge_ev->response_type);
        break;
    }
}

static void handle_xi2_event(struct window_x11 *win,
    xcb_ge_generic_event_t *ge_ev)
{
    const union {
        xcb_input_key_press_event_t *key_press;
        xcb_input_key_press_event_t *key_release;
        xcb_input_button_press_event_t *button_press;
        xcb_input_button_release_event_t *button_release;
        xcb_input_motion_event_t *motion;
        xcb_ge_generic_event_t *generic;
    } ev = { .generic = ge_ev };

    struct x11_registered_input_obj *const keyboard_obj =
        &win->input.registered_input_objs[X11_INPUT_REG_KEYBOARD];
    struct x11_registered_input_obj *const mouse_obj =
        &win->input.registered_input_objs[X11_INPUT_REG_MOUSE];
    const bool keyboard_usable = atomic_load(&keyboard_obj->active_);
    const bool mouse_usable = atomic_load(&mouse_obj->active_);

    struct mouse_x11 *const mouse = mouse_obj->data.mouse;
    struct keyboard_x11 *const keyboard = keyboard_obj->data.keyboard;

    u32 button_bits; /* used in mouse event cases */

    switch (ge_ev->event_type) {
    case XCB_INPUT_KEY_PRESS:
        if (!keyboard_usable)
            break;

        const xcb_keysym_t press_keysym = win->xcb.xcb_key_symbols_get_keysym(
            win->input.key_symbols,
            ev.key_press->detail,
            0
        );
        keyboard_X11_store_key_event(keyboard,
            press_keysym, KEYBOARD_X11_PRESS);

        break;
    case XCB_INPUT_KEY_RELEASE:
        if (!keyboard_usable)
            break;
        if (ev.key_release->deviceid != win->input.master_keyboard_id)
            break;

        const xcb_keysym_t release_keysym = win->xcb.xcb_key_symbols_get_keysym(
            win->input.key_symbols,
            ev.key_release->detail,
            0
        );
        keyboard_X11_store_key_event(keyboard,
            release_keysym, KEYBOARD_X11_RELEASE);

        break;
    case XCB_INPUT_BUTTON_PRESS:
        if (!mouse_usable)
            break;
        if (ev.button_press->deviceid != win->input.master_mouse_id)
            break;

        button_bits = atomic_load(&mouse->button_bits);

        switch (ev.button_press->detail) {
            case 1: button_bits |= P_MOUSE_LEFTBUTTONMASK; break;
            case 2: button_bits |= P_MOUSE_MIDDLEBUTTONMASK; break;
            case 3: button_bits |= P_MOUSE_RIGHTBUTTONMASK; break;
            default:
                break;
        }

        atomic_store(&mouse->button_bits, button_bits);

        break;
    case XCB_INPUT_BUTTON_RELEASE:
        if (!mouse_usable)
            break;
        if (ev.button_release->deviceid != win->input.master_mouse_id)
            break;

        button_bits = atomic_load(&mouse->button_bits);

        switch (ev.button_release->detail) {
            case 1: button_bits &= ~P_MOUSE_LEFTBUTTONMASK; break;
            case 2: button_bits &= ~P_MOUSE_MIDDLEBUTTONMASK; break;
            case 3: button_bits &= ~P_MOUSE_RIGHTBUTTONMASK; break;
            default:
                break;
        }

        atomic_store(&mouse->button_bits, button_bits);

        break;
    case XCB_INPUT_MOTION:
        if (!mouse_usable)
            break;
        if (ev.button_release->deviceid != win->input.master_mouse_id)
            break;

        atomic_store(&mouse->x, u_fp1616_to_f32(ev.motion->event_x));
        atomic_store(&mouse->y, u_fp1616_to_f32(ev.motion->event_y));

        break;
    default:
        break;
    }
}

static void handle_present_event(struct window_x11 *win,
    xcb_ge_generic_event_t *ge_ev)
{
    /* We assume that the window is using software rendering
     * with `PRESENT_PIXMAP` buffers */

    const union {
        xcb_present_complete_notify_event_t *complete;
        xcb_ge_generic_event_t *generic;
    } ev = { .generic = ge_ev };

    struct x11_render_shared_present_data *const shared_data =
        &win->render.sw.shared_buf_data.present;

    switch (ge_ev->event_type) {
    case XCB_PRESENT_COMPLETE_NOTIFY:
    {
        spinlock_acquire(&win->render.sw.swap_lock);
        if (win->generic_info_p->gpu_acceleration != P_WINDOW_ACCELERATION_NONE
            || !win->render.sw.initialized_
            || win->render.sw.curr_front_buf->type != X11_SWFB_PRESENT_PIXMAP)
        {
            s_log_warn("PRESENT_COMPLETE_NOTIFY event received " "while the "
                "front buffer is not an X11_SWFB_PRESENT_PIXMAP; ignoring");
            spinlock_release(&win->render.sw.swap_lock);
            break;
        }
        spinlock_release(&win->render.sw.swap_lock);

        const u32 stored_serial = atomic_load(&shared_data->serial);
        if (stored_serial != ev.complete->serial) {
            if (stored_serial > ev.complete->serial) {
                s_log_error("Something has gone seriously wrong "
                    "with the present serial value (%u) - should be %u",
                    stored_serial, ev.complete->serial);
            } else if (stored_serial < ev.complete->serial) {
                /* Send the missing page flip events */
                const u32 diff = ev.complete->serial - stored_serial;
                s_log_warn("Dropped %u frame(s)", diff);
                for (u32 i = 0; i < diff - 1; i++) {
                    p_event_send(&(const struct p_event) {
                        .type = P_EVENT_PAGE_FLIP,
                        .info.page_flip_status = 1,
                    });
                }
            }
            X11_render_software_finish_frame(&win->render.sw, 1);
        } else {
            X11_render_software_finish_frame(&win->render.sw, 0);
        }
        break;
    }
    default:
        break;
    }
}

static void handle_shm_completion_event(struct window_x11 *win,
    xcb_generic_event_t *ev)
{
    struct x11_render_shared_shm_data *const shared_data =
        &win->render.sw.shared_buf_data.shm;
    const u64 stored_sequence_number =
        atomic_load(&shared_data->blit_request_sequence_number);

    if (ev->full_sequence != stored_sequence_number) {
        s_log_error("The last ShmPutImage request's sequence number (%lu) "
            "doesn't match that of the completion event (%lu); "
            "some frames were possibly dropped",
            stored_sequence_number, ev->full_sequence
        );
        p_event_send(&(const struct p_event) {
            .type = P_EVENT_PAGE_FLIP,
            .info.page_flip_status = 1,
        });

        /* This event arriving always means that SOME page flip
         * has succeeded, so apart from notifying about the mismatch
         * we should also finish the frame as if everything was OK */
    }

    X11_render_software_finish_frame(&win->render.sw, 0);
}

static void handle_shm_error(struct window_x11 *win, xcb_generic_error_t *e)
{
    struct x11_render_shared_shm_data *const shared_data =
        &win->render.sw.shared_buf_data.shm;

    if (e->full_sequence ==
            atomic_load(&shared_data->blit_request_sequence_number)
    ) {
        s_log_error("A ShmPutImage frame presentation failed (frame dropped)");
        p_event_send(&(const struct p_event) {
            .type = P_EVENT_PAGE_FLIP,
            .info.page_flip_status = 1,
        });
    }
    s_log_error("XShm error %u (request opcode: %u)",
        e->error_code, e->minor_code);
}
