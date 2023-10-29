#include "event-listener.h"
#include "../user-input/pressable-obj.h"
#include "on-event.h"
#include <stdio.h>

evl_EventListener* evl_initEventListener(evl_Target target, oe_OnEvent *onEvent, evl_EventListenerConfig *cfg)
{
    evl_EventListener *evl = malloc(sizeof(evl_EventListener));

    evl->type = cfg->type;
    evl->detected = false;

    evl->onEvent.fn = onEvent->fn;
    evl->onEvent.argc = onEvent->argc;
    evl->onEvent.argv = onEvent->argv;

    switch(cfg->type){
        case EVL_EVENT_KEYBOARD_KEYPRESS:
            evl->objectPtr = &(*target.keyboard)[cfg->targetInfo.keycode].key->pressed;
        	break;
        case EVL_EVENT_KEYBOARD_KEYDOWN:
            evl->objectPtr = &(*target.keyboard)[cfg->targetInfo.keycode].key->down;
        	break;
        case EVL_EVENT_KEYBOARD_KEYUP:
            evl->objectPtr = &(*target.keyboard)[cfg->targetInfo.keycode].key->up;
        	break;
        case EVL_EVENT_MOUSE_BUTTONPRESS:
            switch(cfg->targetInfo.buttonMask){
                default:
                case MS_LEFTBUTTONMASK:
                    evl->objectPtr = &target.mouse->buttonLeft->pressed;
                    break;
                case MS_RIGHTBUTTONMASK:
                    evl->objectPtr = &target.mouse->buttonRight->pressed;
                    break;
                case MS_MIDDLEBUTTONMASK:
                    evl->objectPtr = &target.mouse->buttonMiddle->pressed;
                    break;
            }
            break;
        case EVL_EVENT_MOUSE_BUTTONDOWN:
            switch(cfg->targetInfo.buttonMask){
                default:
                case MS_LEFTBUTTONMASK:
                    evl->objectPtr = &target.mouse->buttonLeft->down;
                    break;
                case MS_RIGHTBUTTONMASK:
                    evl->objectPtr = &target.mouse->buttonRight->down;
                    break;
                case MS_MIDDLEBUTTONMASK:
                    evl->objectPtr = &target.mouse->buttonMiddle->down;
                    break;
            }
        case EVL_EVENT_MOUSE_BUTTONUP:
            switch(cfg->targetInfo.buttonMask){
                default:
                case MS_LEFTBUTTONMASK:
                    evl->objectPtr = &target.mouse->buttonLeft->up;
                    break;
                case MS_RIGHTBUTTONMASK:
                    evl->objectPtr = &target.mouse->buttonRight->up;
                    break;
                case MS_MIDDLEBUTTONMASK:
                    evl->objectPtr = &target.mouse->buttonMiddle->up;
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
    
    if(evl->detected)
        oe_executeOnEventfn(evl->onEvent);
}

void evl_destroyEventListener(evl_EventListener *evl)
{
    if(evl->onEvent.argv != NULL)
        fputs("WARNING (from evl_destroyEventListener): evl->onEvent->argv was not set to NULL",stderr);

    free(evl);
    evl = NULL;
}
