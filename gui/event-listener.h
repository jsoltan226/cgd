#ifndef EVENT_LISTENER_H
#define EVENT_LISTENER_H

#include "on-event.h"
#include <platform/keyboard.h>
#include <platform/mouse.h>
#include <stdbool.h>

enum event_listener_type {
    EVL_EVENT_KEYBOARD_KEYPRESS,
    EVL_EVENT_KEYBOARD_KEYDOWN,
    EVL_EVENT_KEYBOARD_KEYUP,
    EVL_EVENT_MOUSE_BUTTONPRESS,
    EVL_EVENT_MOUSE_BUTTONDOWN,
    EVL_EVENT_MOUSE_BUTTONUP,
};

struct event_listener {
    enum event_listener_type type;
    struct on_event_obj on_event_obj;
    const bool *obj_ptr;
    bool detected;
};

struct event_listener_config {
    enum event_listener_type type;
    union event_listener_target {
        const struct p_keyboard **keyboard_p;
        const struct p_mouse **mouse_p;
    } target_obj;
    union event_listener_target_info {
        enum p_mouse_button button_type;
        enum p_keyboard_keycode keycode;
    } target_info;

    struct on_event_obj on_event;
};

struct event_listener *
event_listener_init(const struct event_listener_config *cfg);

void event_listener_update(struct event_listener *evl);

void event_listener_destroy(struct event_listener **evl_p);

#endif
