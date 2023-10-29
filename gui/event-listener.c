#include "event-listener.h"
#include "../user-input/pressable-obj.h"

evl_EventListener* evl_initEventListener(evl_Target target, evl_EventListenerConfig *cfg)
{
    evl_EventListener *evl = malloc(sizeof(evl_EventListener));

    evl->type = cfg->type;
    evl->detected = false;

    switch(cfg->type){
        case EVL_EVENT_KEYBOARD_KEYPRESS:
            evl->objectPtr = &target.keyboard[cfg->targetInfo.keycode].key.pressed;
        	break;
        case EVL_EVENT_KEYBOARD_KEYDOWN:
            evl->objectPtr = &target.keyboard[cfg->targetInfo.keycode].key.down;
        	break;
        case EVL_EVENT_KEYBOARD_KEYUP:
            evl->objectPtr = &target.keyboard[cfg->targetInfo.keycode].key.up;
        	break;
        case EVL_EVENT_MOUSE_BUTTONPRESS:
            switch(cfg->targetInfo.buttonMask){
                default:
                case INPUT_MOUSE_LEFTBUTTONMASK:
                    evl->objectPtr = &target.mouse->buttonLeft.pressed;
                    break;
                case INPUT_MOUSE_RIGHTBUTTONMASK:
                    evl->objectPtr = &target.mouse->buttonRight.pressed;
                    break;
                case INPUT_MOUSE_MIDDLEBUTTONMASK:
                    evl->objectPtr = &target.mouse->buttonMiddle.pressed;
                    break;
            }
            break;
        case EVL_EVENT_MOUSE_BUTTONDOWN:
            switch(cfg->targetInfo.buttonMask){
                default:
                case INPUT_MOUSE_LEFTBUTTONMASK:
                    evl->objectPtr = &target.mouse->buttonLeft.down;
                    break;
                case INPUT_MOUSE_RIGHTBUTTONMASK:
                    evl->objectPtr = &target.mouse->buttonRight.down;
                    break;
                case INPUT_MOUSE_MIDDLEBUTTONMASK:
                    evl->objectPtr = &target.mouse->buttonMiddle.down;
                    break;
            }
        case EVL_EVENT_MOUSE_BUTTONUP:
            switch(cfg->targetInfo.buttonMask){
                default:
                case INPUT_MOUSE_LEFTBUTTONMASK:
                    evl->objectPtr = &target.mouse->buttonLeft.up;
                    break;
                case INPUT_MOUSE_RIGHTBUTTONMASK:
                    evl->objectPtr = &target.mouse->buttonRight.up;
                    break;
                case INPUT_MOUSE_MIDDLEBUTTONMASK:
                    evl->objectPtr = &target.mouse->buttonMiddle.up;
                    break;
            }
        	break;
    }

    return evl;
}

void evl_updateEventListener(evl_EventListener *evl)
{
    if(evl->objectPtr)
        evl->detected = *evl->objectPtr;
}

void evl_destroyEventListener(evl_EventListener *evl)
{
    free(evl);
    evl = NULL;
}
