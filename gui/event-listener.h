#ifndef EVENT_LISTENER_H
#define EVENT_LISTENER_H

#include <stdbool.h>
#include "input/keyboard.h"
#include "input/mouse.h"
#include "on-event.h"

typedef enum {
    EVL_EVENT_KEYBOARD_KEYPRESS,
    EVL_EVENT_KEYBOARD_KEYDOWN,
    EVL_EVENT_KEYBOARD_KEYUP,
    EVL_EVENT_MOUSE_BUTTONPRESS,
    EVL_EVENT_MOUSE_BUTTONDOWN,
    EVL_EVENT_MOUSE_BUTTONUP,
} evl_EventType;

typedef struct {
    evl_EventType type;
    oe_OnEvent onEvent;
    bool *objectPtr;
    bool detected;
} evl_EventListener;

typedef struct {
    evl_EventType type;
    union {
        enum mouse_button button_type;
        enum keyboard_keycode keycode;
    } targetInfo;
} evl_EventListenerConfig;

typedef struct {
    struct mouse *mouse;
    struct keyboard *keyboard;
} evl_Target;

evl_EventListener* evl_initEventListener(evl_EventListenerConfig *cfg, oe_OnEvent *oeObj, evl_Target *t);
void evl_updateEventListener(evl_EventListener *evl);
void evl_destroyEventListener(evl_EventListener *evl);

#endif
