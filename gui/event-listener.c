#include "event-listener.h"
#include "on-event.h"
#include "input/pressable-obj.h"
#include <stdio.h>
#include <assert.h>

evl_EventListener* evl_initEventListener(evl_EventListenerConfig *cfg, oe_OnEvent *onEventObj, evl_Target *t)
{
    evl_EventListener *evl = malloc(sizeof(evl_EventListener));
    assert(evl != NULL);

    evl->onEvent.fn = onEventObj->fn;
    evl->onEvent.argc = onEventObj->argc;
    memcpy(evl->onEvent.argv, onEventObj->argv, OE_ARGV_SIZE * sizeof(void*));

    /* Copy other info given in configuration */
    evl->type = cfg->type;
    evl->detected = false;

    evl->objectPtr = NULL;
    switch(cfg->type){
        case EVL_EVENT_KEYBOARD_KEYPRESS:
            evl->objectPtr = &(kb_getKey(t->keyboard, cfg->targetInfo.keycode).pressed);
        	break;
        case EVL_EVENT_KEYBOARD_KEYDOWN:
            evl->objectPtr = &(kb_getKey(t->keyboard, cfg->targetInfo.keycode).down);
        	break;
        case EVL_EVENT_KEYBOARD_KEYUP:
            evl->objectPtr = &(kb_getKey(t->keyboard, cfg->targetInfo.keycode).up);
        	break;
        case EVL_EVENT_MOUSE_BUTTONPRESS:
            evl->objectPtr = &t->mouse->buttons[cfg->targetInfo.button_type].pressed;
            break;
        case EVL_EVENT_MOUSE_BUTTONDOWN:
            evl->objectPtr = &t->mouse->buttons[cfg->targetInfo.button_type].down;
            break;
        case EVL_EVENT_MOUSE_BUTTONUP:
            evl->objectPtr = &t->mouse->buttons[cfg->targetInfo.button_type].up;
        	break;
    }

    assert(evl->objectPtr != NULL);

    return evl;
}

void evl_updateEventListener(evl_EventListener *evl)
{
    /* It should't be possible for evl->objectPtr to be NULL, so I am not checking for that */
    evl->detected = *evl->objectPtr;
    
    if(evl->detected && evl->onEvent.fn)
        oe_executeOnEventfn(evl->onEvent);
}

void evl_destroyEventListener(evl_EventListener *evl)
{
    free(evl);
    evl = NULL;
}
