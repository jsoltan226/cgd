#include "event-listener.h"
#include "on-event.h"
#include <core/log.h>
#include <core/util.h>
#include <core/pressable-obj.h>
#include <platform/keyboard.h>
#include <platform/mouse.h>
#include <stdlib.h>
#include <string.h>

#define MODULE_NAME "event-listener"

struct event_listener *
event_listener_init(const struct event_listener_config *cfg)
{
    u_check_params(cfg != NULL &&
        /* cfg->target_obj is a union of 2 pointers,
         * so we only need to check if one field is invalid */
        cfg->target_obj.keyboard_p != NULL &&
        *cfg->target_obj.keyboard_p != NULL
    );

    struct event_listener *evl = malloc(sizeof(struct event_listener));
    s_assert(evl != NULL, "malloc() failed for struct event_listener");

    evl->on_event_obj.fn = cfg->on_event.fn;
    memcpy(evl->on_event_obj.arg, cfg->on_event.arg, ONEVENT_OBJ_ARG_SIZE);

    evl->type = cfg->type;
    evl->detected = false;

    const struct p_keyboard *kb = *cfg->target_obj.keyboard_p;
    const struct p_mouse *m = *cfg->target_obj.mouse_p;

    const union event_listener_target_info *const i = &cfg->target_info;

    evl->obj_ptr = NULL;
    switch (cfg->type) {
        case EVL_EVENT_KEYBOARD_KEYPRESS:
            evl->obj_ptr = &p_keyboard_get_key(kb, i->keycode)->pressed;
            break;
        case EVL_EVENT_KEYBOARD_KEYDOWN:
            evl->obj_ptr = &p_keyboard_get_key(kb, i->keycode)->down;
            break;
        case EVL_EVENT_KEYBOARD_KEYUP:
            evl->obj_ptr = &p_keyboard_get_key(kb, i->keycode)->up;
            break;
        case EVL_EVENT_MOUSE_BUTTONPRESS:
            evl->obj_ptr = &p_mouse_get_button(m, i->button_type)->pressed;
            break;
        case EVL_EVENT_MOUSE_BUTTONDOWN:
            evl->obj_ptr = &p_mouse_get_button(m, i->button_type)->down;
            break;
        case EVL_EVENT_MOUSE_BUTTONUP:
            evl->obj_ptr = &p_mouse_get_button(m, i->button_type)->up;
            break;
    }

    return evl;
}

void event_listener_update(struct event_listener *evl)
{
    u_check_params(evl != NULL);

    evl->detected = *evl->obj_ptr;

    if(evl->detected && evl->on_event_obj.fn)
        on_event_execute(evl->on_event_obj);
}

void event_listener_destroy(struct event_listener **evl_p)
{
    if (evl_p == NULL || *evl_p == NULL) return;

    u_nzfree(evl_p);
}
