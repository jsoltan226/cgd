#include "event-listener.h"
#include <cgd/user-input/pressable-obj.h>
#include <cgd/gui/on-event.h>
#include <stdio.h>
#include <assert.h>

evl_EventListener* evl_initEventListener(evl_EventListenerConfig *cfg, oe_OnEvent *onEventObj, evl_Target *t)
{
    /* Allocate space for the event listener struct */
    evl_EventListener *evl = malloc(sizeof(evl_EventListener));
    assert(evl != NULL);

    /* Copy over the info from the 'onEventObj' argument */
    evl->onEvent.fn = onEventObj->fn;
    evl->onEvent.argc = onEventObj->argc;
    assert( memcpy(evl->onEvent.argv, onEventObj->argv, OE_ARGV_SIZE * sizeof(void*)) );

    /* Copy other info given in configuration */
    evl->type = cfg->type;
    evl->detected = false;

    evl->objectPtr = NULL;
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

    assert(evl->objectPtr != NULL);

    return evl;
}

void evl_updateEventListener(evl_EventListener *evl)
{
    /* Update the event listener. It should't be possible for evl->objectPtr to be NULL, so I am not checking for that */
    evl->detected = *evl->objectPtr;
    
    /* If the button is pressed and the onClick function pointer is valid, execute the onClick function */
    if(evl->detected && evl->onEvent.fn)
        oe_executeOnEventfn(evl->onEvent);
}

void evl_destroyEventListener(evl_EventListener *evl)
{
    free(evl);
    evl = NULL;
}
