#include "../mouse.h"
#include "core/log.h"
#include "core/math.h"
#include <core/int.h>
#include <core/pressable-obj.h>
#include <core/vector.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/poll.h>
#include <linux/input.h>
#include <linux/input-event-codes.h>
#define P_INTERNAL_GUARD__
#include "evdev.h"
#undef P_INTERNAL_GUARD__
#define P_INTERNAL_GUARD__
#include "mouse-evdev.h"
#undef P_INTERNAL_GUARD__
#define P_INTERNAL_GUARD__
#include "mouse-internal.h"
#undef P_INTERNAL_GUARD__

#define MODULE_NAME "mouse-evdev"

static void read_mouse_events_from_evdev(i32 fd,
    pressable_obj_t buttons[P_MOUSE_N_BUTTONS],
    bool updated_buttons[P_MOUSE_N_BUTTONS],
    f32 *x, f32 *y);

i32 mouse_evdev_init(struct mouse_evdev *mouse, u32 flags)
{
    memset(mouse, 0, sizeof(struct mouse_evdev));

    mouse->mouse_devs = evdev_find_and_load_devices(EVDEV_MOUSE);
    if (mouse->mouse_devs == NULL)
        goto err;

    mouse->poll_fds = vector_new(struct pollfd);
    vector_reserve(mouse->poll_fds, vector_size(mouse->mouse_devs));

    for (u32 i = 0; i < vector_size(mouse->mouse_devs); i++) {
        vector_push_back(mouse->poll_fds, (struct pollfd) {
            .fd = mouse->mouse_devs[i].fd,
            .events = POLLIN,
            .revents = 0
        });
    }

    return 0;

err:
    mouse_evdev_destroy(mouse);
    return 1;
}

void mouse_evdev_update(struct p_mouse *mouse)
{
    u32 n_mouse_devs = vector_size(mouse->evdev.mouse_devs);
    for (u32 i = 0; i < n_mouse_devs; i++) {
        if (poll(mouse->evdev.poll_fds, n_mouse_devs, 0) == -1) {
            s_log_error("Failed to poll() on mouse event device %s: %s",
                    mouse->evdev.mouse_devs[i].path, strerror(errno));
            return;
        }

        bool updated_buttons[P_MOUSE_N_BUTTONS] = { 0 };
        for (u32 i = 0; i < n_mouse_devs; i++) {
            struct pollfd *curr_pollfd = &mouse->evdev.poll_fds[i];
            struct evdev *curr_evdev = &mouse->evdev.mouse_devs[i];

            if (!(curr_pollfd->revents & POLLIN)) continue;

            read_mouse_events_from_evdev(curr_evdev->fd,
                    mouse->buttons, updated_buttons,
                    &mouse->pos.x, &mouse->pos.y);

            for (u32 i = 0; i < P_MOUSE_N_BUTTONS; i++) {
                if (!updated_buttons[i] &&
                    (mouse->buttons[i].pressed || mouse->buttons[i].up)
                ) {
                    pressable_obj_update(&mouse->buttons[i],
                        mouse->buttons[i].pressed);
                }
            }

            /* Update mouse out-of-window status */
            const rect_t
            mouse_rect = {
                .x = mouse->pos.x,
                .y = mouse->pos.y,
                .w = 0,
                .h = 0,
            },
            window_rect = {
                .x = mouse->win->x,
                .y = mouse->win->y,
                .w = mouse->win->w,
                .h = mouse->win->h,
            };
            mouse->is_out_of_window = !u_collision(&mouse_rect, &window_rect);
        }
    }
}

void mouse_evdev_destroy(struct mouse_evdev *mouse)
{
    if (mouse == NULL) return;

    if (mouse->mouse_devs != NULL) {
        for (u32 i = 0; i < vector_size(mouse->mouse_devs); i++) {
            if (mouse->mouse_devs[i].fd != -1) {
                close(mouse->mouse_devs[i].fd);
                mouse->mouse_devs[i].fd = -1;
            }
        }
        vector_destroy(mouse->mouse_devs);
        vector_destroy(mouse->poll_fds);
    }
}

static void read_mouse_events_from_evdev(i32 fd,
    pressable_obj_t buttons[P_MOUSE_N_BUTTONS],
    bool updated_buttons[P_MOUSE_N_BUTTONS],
    f32 *x, f32 *y)
{
    struct input_event ev = { 0 };
    i32 n_bytes_read = 0;
    while (n_bytes_read = read(fd, &ev, sizeof(struct input_event)),
        n_bytes_read > 0
    ) {
        if (n_bytes_read <= 0) { /* Either no data was read or some other error */
            return;
        } else if (n_bytes_read != sizeof(struct input_event)) {
            s_log_fatal(MODULE_NAME, __func__, 
                    "Read %i bytes from event device, expected size is %i. "
                    "The linux input driver is probably broken...",
                    n_bytes_read, sizeof(struct input_event));
        }

        enum p_mouse_button button = -1;

        if (ev.type == EV_REL) {
            if (ev.code == REL_X) {
                *x += (f32)ev.value;
            } else if(ev.code == REL_Y) {
                *y += (f32)ev.value;
            }
            continue;
        } else if (ev.type == EV_KEY) {
            switch (ev.code) {
                case BTN_LEFT: button = P_MOUSE_BUTTON_LEFT; break;
                case BTN_RIGHT: button = P_MOUSE_BUTTON_RIGHT; break;
                case BTN_MIDDLE: button = P_MOUSE_BUTTON_MIDDLE; break;
                default:
                    continue;
            }
        } else {
            continue;
        }

        /* ev.value = 0: key released
         * ev.value = 1: key pressed
         * ev.value = 2: key held
         */
        pressable_obj_update(&buttons[button], ev.value > 0);
        updated_buttons[button] = true;

        /*
         * s_log_debug("Key event %i for keycode %s",
         * ev.value, p_keyboard_keycode_strings[p_kb_keycode]);
         */
    }
}
