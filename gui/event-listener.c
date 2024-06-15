#include "event-listener.h"
#include "on-event.h"
#include "core/log.h"
#include "input/pressable-obj.h"
#include <stdio.h>

struct event_listener * event_listener_init(
    struct event_listener_config *cfg,
    struct on_event_obj *on_event_obj,
    struct event_listener_target *t
)
{
    struct event_listener *evl = malloc(sizeof(struct event_listener));
    if (evl == NULL) {
        s_log_error("event-listener", "malloc() failed for struct event_listener");
        return NULL;
    }

    evl->on_event_obj.fn = on_event_obj->fn;
    memcpy(evl->on_event_obj.argv_buf, on_event_obj->argv_buf, ONEVENT_OBJ_ARGV_SIZE);

    /* Copy other info given in configuration */
    evl->type = cfg->type;
    evl->detected = false;

    evl->obj_ptr = NULL;
    switch(cfg->type){
        case EVL_EVENT_KEYBOARD_KEYPRESS:
            evl->obj_ptr = &(kb_getKey(t->keyboard, cfg->target_info.keycode).pressed);
            break;
        case EVL_EVENT_KEYBOARD_KEYDOWN:
            evl->obj_ptr = &(kb_getKey(t->keyboard, cfg->target_info.keycode).down);
            break;
        case EVL_EVENT_KEYBOARD_KEYUP:
            evl->obj_ptr = &(kb_getKey(t->keyboard, cfg->target_info.keycode).up);
            break;
        case EVL_EVENT_MOUSE_BUTTONPRESS:
            evl->obj_ptr = &t->mouse->buttons[cfg->target_info.button_type].pressed;
            break;
        case EVL_EVENT_MOUSE_BUTTONDOWN:
            evl->obj_ptr = &t->mouse->buttons[cfg->target_info.button_type].down;
            break;
        case EVL_EVENT_MOUSE_BUTTONUP:
            evl->obj_ptr = &t->mouse->buttons[cfg->target_info.button_type].up;
            break;
    }

    return evl;
}

void event_listener_update(struct event_listener *evl)
{
    if (evl == NULL) return;

    /* It should't be possible for evl->objectPtr to be NULL, so I am not checking for that */
    evl->detected = *evl->obj_ptr;

    if(evl->detected && evl->on_event_obj.fn)
        on_event_execute(evl->on_event_obj);
}

void event_listener_destroy(struct event_listener *evl)
{
    if (evl == NULL) return;

    free(evl);
}
