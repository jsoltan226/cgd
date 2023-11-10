#include "event-listener.h"
#include "../user-input/pressable-obj.h"
#include "on-event.h"
#include <stdio.h>

evl_EventListener* evl_initEventListener(evl_EventListenerConfig *cfg, oe_OnEvent *onEventObj, evl_Target *t)
{
    evl_EventListener *evl = malloc(sizeof(evl_EventListener));

    evl->type = cfg->type;
    evl->detected = false;

    evl->onEvent.fn = onEventObj->fn;
    evl->onEvent.argc = onEventObj->argc;
    evl->onEvent.argv = onEventObj->argv;

    switch(cfg->type){
        case EVL_EVENT_KEYBOARD_KEYPRESS:
            evl->objectPtr = &(kb_getKey(t->keyboard, cfg->targetInfo.keycode)->pressed);
        	break;
        case EVL_EVENT_KEYBOARD_KEYDOWN:
            evl->objectPtr = &(kb_getKey(t->keyboard, cfg->targetInfo.keycode)->down);
        	break;
        case EVL_EVENT_KEYBOARD_KEYUP:
            evl->objectPtr = &(kb_getKey(t->keyboard, cfg->targetInfo.keycode)->up);
        	break;
        case EVL_EVENT_MOUSE_BUTTONPRESS:
            switch(cfg->targetInfo.buttonMask){
                default:
                case MS_LEFTBUTTONMASK:
                    evl->objectPtr = &(t->mouse->buttonLeft->pressed);
                    break;
                case MS_RIGHTBUTTONMASK:
                    evl->objectPtr = &(t->mouse->buttonRight->pressed);
                    break;
                case MS_MIDDLEBUTTONMASK:
                    evl->objectPtr = &(t->mouse->buttonMiddle->pressed);
                    break;
            }
            break;
        case EVL_EVENT_MOUSE_BUTTONDOWN:
            switch(cfg->targetInfo.buttonMask){
                default:
                case MS_LEFTBUTTONMASK:
                    evl->objectPtr = &(t->mouse->buttonLeft->down);
                    break;
                case MS_RIGHTBUTTONMASK:
                    evl->objectPtr = &(t->mouse->buttonRight->down);
                    break;
                case MS_MIDDLEBUTTONMASK:
                    evl->objectPtr = &(t->mouse->buttonMiddle->down);
                    break;
            }
        case EVL_EVENT_MOUSE_BUTTONUP:
            switch(cfg->targetInfo.buttonMask){
                default:
                case MS_LEFTBUTTONMASK:
                    evl->objectPtr = &(t->mouse->buttonLeft->up);
                    break;
                case MS_RIGHTBUTTONMASK:
                    evl->objectPtr = &(t->mouse->buttonRight->up);
                    break;
                case MS_MIDDLEBUTTONMASK:
                    evl->objectPtr = &(t->mouse->buttonMiddle->up);
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
    oe_destroyOnEventObj(evl->onEvent);
    free(evl);
    evl = NULL;
}
