#ifndef EVENT_LISTENER_H
#define EVENT_LISTENER_H

#include <stdbool.h>
#include <cgd/user-input/keyboard.h>
#include <cgd/user-input/mouse.h>
#include <cgd/gui/on-event.h>

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
        ms_Mouse_ButtonMask buttonMask;
        kb_KeyCode keycode;
    } targetInfo;
} evl_EventListenerConfig;

typedef struct {
    ms_Mouse *mouse;
    kb_Keyboard *keyboard;
} evl_Target;

evl_EventListener* evl_initEventListener(evl_EventListenerConfig *cfg, oe_OnEvent *oeObj, evl_Target *t);
void evl_updateEventListener(evl_EventListener *evl);
void evl_destroyEventListener(evl_EventListener *evl);

#endif
