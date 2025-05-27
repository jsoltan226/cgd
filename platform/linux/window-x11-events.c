#define _GNU_SOURCE
#include "../event.h"
#include "../mouse.h"
#include "../window.h"
#include <core/log.h>
#include <core/int.h>
#include <core/math.h>
#include <core/util.h>
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

    if (acceleration == P_WINDOW_ACCELERATION_NONE &&
        shared_buf_data->shm.initialized_ &&
        XCB_EVENT_RESPONSE_TYPE(ev) ==
            shared_buf_data->shm.ext_data->first_event + XCB_SHM_COMPLETION
        )
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
                shared_buf_data->shm.ext_data->major_opcode)
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

end:

    /* If we get a notification that the keyboard and/or mouse
     * are being deregistered, acknowledge it */
    if (atomic_load(&win->input.keyboard_deregistration_notify))
        pthread_cond_signal(&win->input.keyboard_deregistration_ack);

    if (atomic_load(&win->input.mouse_deregistration_notify))
        pthread_cond_signal(&win->input.mouse_deregistration_ack);
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

    if (acceleration == P_WINDOW_ACCELERATION_NONE &&
        shared_buf_data->present.initialized_ &&
        ge_ev->extension == shared_buf_data->present.ext_data->major_opcode)
    {
        handle_present_event(win, ge_ev);
    } else if (ge_ev->extension == win->input.xinput_ext_data->major_opcode) {
        handle_xi2_event(win, ge_ev);
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

    const bool keyboard_unusable =
        !(win->input.registered_keyboard) ||
        atomic_load(&win->input.keyboard_deregistration_notify);

    /* Used in mouse event cases */
    const bool mouse_unusable =
        !(win->input.registered_mouse) ||
        atomic_load(&win->input.mouse_deregistration_notify);

    struct mouse_x11 *mouse = win->input.registered_mouse;
    u32 button_bits;

    switch (ge_ev->event_type) {
    case XCB_INPUT_KEY_PRESS:
        if (keyboard_unusable)
            break;
        if (!win->input.registered_keyboard)
            break;

        const xcb_keysym_t press_keysym = win->xcb.xcb_key_symbols_get_keysym(
            win->input.key_symbols,
            ev.key_press->detail,
            0
        );
        X11_keyboard_store_key_event(win->input.registered_keyboard,
            press_keysym, KEYBOARD_X11_PRESS);

        break;
    case XCB_INPUT_KEY_RELEASE:
        if (keyboard_unusable)
            break;
        if (ev.key_release->deviceid != win->input.master_keyboard_id)
            break;

        const xcb_keysym_t release_keysym = win->xcb.xcb_key_symbols_get_keysym(
            win->input.key_symbols,
            ev.key_release->detail,
            0
        );
        X11_keyboard_store_key_event(win->input.registered_keyboard,
            release_keysym, KEYBOARD_X11_RELEASE);

        break;
    case XCB_INPUT_BUTTON_PRESS:
        if (mouse_unusable)
            break;
        if (ev.button_press->deviceid != win->input.master_mouse_id)
            break;

        button_bits = atomic_load(&mouse->button_bits);

        if (ev.button_press->detail == 1)
            button_bits |= P_MOUSE_LEFTBUTTONMASK;
        if (ev.button_press->detail == 2)
            button_bits |= P_MOUSE_MIDDLEBUTTONMASK;
        if (ev.button_press->detail == 3)
            button_bits |= P_MOUSE_RIGHTBUTTONMASK;

        atomic_store(&mouse->button_bits, button_bits);

        break;
    case XCB_INPUT_BUTTON_RELEASE:
        if (mouse_unusable)
            break;
        if (ev.button_release->deviceid != win->input.master_mouse_id)
            break;

        button_bits = atomic_load(&mouse->button_bits);

        if (ev.button_release->detail == 1)
            button_bits &= ~P_MOUSE_LEFTBUTTONMASK;
        if (ev.button_release->detail == 2)
            button_bits &= ~P_MOUSE_MIDDLEBUTTONMASK;
        if (ev.button_release->detail == 3)
            button_bits &= ~P_MOUSE_RIGHTBUTTONMASK;

        atomic_store(&mouse->button_bits, button_bits);

        break;
    case XCB_INPUT_MOTION:
        if (mouse_unusable)
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
    case XCB_PRESENT_COMPLETE_NOTIFY: {
        if (win->generic_info_p->gpu_acceleration != P_WINDOW_ACCELERATION_NONE
            || !win->render.sw.initialized_
            || win->render.sw.curr_front_buf->type != X11_SWFB_PRESENT_PIXMAP)
        {
            s_log_warn("PRESENT_COMPLETE_NOTIFY event received " "while the "
                "front buffer is not an X11_SWFB_PRESENT_PIXMAP; ignoring");
            break;
        }

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
