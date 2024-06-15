#ifndef EVENT_LISTENER_H
#define EVENT_LISTENER_H

#include <stdbool.h>
#include "input/keyboard.h"
#include "input/mouse.h"
#include "on-event.h"

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
    bool *obj_ptr;
    bool detected;
};

struct event_listener_config {
    enum event_listener_type type;
    union {
        enum mouse_button button_type;
        enum keyboard_keycode keycode;
    } target_info;
};

struct event_listener_target {
    struct mouse *mouse;
    struct keyboard *keyboard;
};

struct event_listener * event_listener_init(
    struct event_listener_config *cfg,
    struct on_event_obj *on_event_obj,
    struct event_listener_target *target
);
void event_listener_update(struct event_listener *evl);
void event_listener_destroy(struct event_listener *evl);

#endif
