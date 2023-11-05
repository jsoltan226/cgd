#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include "event-listener.h"
#include "menu.h"
#include "buttons.h"
#include "on-event.h"
#include "parallax-bg.h"
#include "sprite.h"

static void mn_initOnEventObj(mn_Menu *mn, oe_OnEvent *oeObj, mn_OnEventCfg *cfg);
static int mn_memCopy(int argc, void **argv);
static int mn_switchMenu(int argc, void **argv);
static int mn_goBack(int argc, void **argv);
static int mn_printMessage(int argc, void **argv);
static int mn_executeOther(int argc, void **argv);

mn_Menu* mn_initMenu(mn_MenuConfig* cfg, SDL_Renderer* renderer, kb_Keyboard *keyboard, ms_Mouse *mouse)
{
    mn_Menu* mn = malloc(sizeof(mn_Menu));
    assert(mn != NULL);

    mn->spriteCount = cfg->spriteCount;
    mn->buttonCount = cfg->buttonCount;
    mn->eventListenerCount = cfg->eventListenerCount;
    mn->sprites = malloc(sizeof(spr_Sprite) * cfg->spriteCount);
    mn->buttons = malloc(sizeof(btn_Button) * cfg->buttonCount);
    mn->eventListeners = malloc(sizeof(evl_EventListener) * cfg->eventListenerCount);
    mn->switchTo = MN_ID_NULL;
    mn->ID = cfg->id;
    mn->statusFlags = MN_STATUS_NONE;
    assert( (mn->sprites != NULL || mn->spriteCount == 0) && 
            (mn->buttons != NULL || mn->buttonCount == 0) &&
            (mn->eventListeners != NULL || mn->eventListenerCount == 0)
          );

    for(int i = 0; i < cfg->eventListenerCount; i++){
        evl_Target t;
        switch(cfg->eventListenerCfgs[i].type){
            default:
            case EVL_EVENT_KEYBOARD_KEYUP: case EVL_EVENT_KEYBOARD_KEYDOWN: case EVL_EVENT_KEYBOARD_KEYPRESS:
                t.keyboard = keyboard;
                break;
            case EVL_EVENT_MOUSE_BUTTONUP: case EVL_EVENT_MOUSE_BUTTONDOWN: case EVL_EVENT_MOUSE_BUTTONPRESS:
                t.mouse = mouse;
                break;
        }
        oe_OnEvent onEventObj;
        mn_initOnEventObj(mn, &onEventObj, &cfg->eventListenerOnEventConfigs[i]);
        mn->eventListeners[i] = evl_initEventListener(t, &onEventObj, &cfg->eventListenerCfgs[i]);
    }

    for(int i = 0; i < cfg->spriteCount; i++){
        mn->sprites[i] = spr_initSprite(&cfg->spriteCfgs[i], renderer);
    }

    for(int i = 0; i < cfg->buttonCount; i++){
        oe_OnEvent onClickObj;
        mn_initOnEventObj(mn, &onClickObj, &cfg->buttonOnClickCfgs[i]);
        mn->buttons[i] = btn_initButton(&cfg->buttonSpriteCfgs[i], &onClickObj, renderer);
    }

    mn->bg = bg_initBG(&cfg->bgConfig, renderer);

    return mn;
}

void mn_updateMenu(mn_Menu* mn, ms_Mouse *mouse)
{
    for(int i = 0; i < mn->buttonCount; i++){
        btn_updateButton(mn->buttons[i], mouse);
    }
    for(int i = 0; i < mn->eventListenerCount; i++){
        evl_updateEventListener(mn->eventListeners[i]);
    }
    bg_updateBG(mn->bg);
}

void mn_drawMenu(mn_Menu* mn, SDL_Renderer* renderer, bool displayButtonHitboxes)
{
    bg_drawBG(mn->bg, renderer);

    for(int i = 0; i < mn->spriteCount; i++){
        spr_drawSprite(mn->sprites[i], renderer);
    }
    for(int i = 0; i < mn->buttonCount; i++){
        btn_drawButton(mn->buttons[i], renderer, displayButtonHitboxes);
    }

}

void mn_destroyMenu(mn_Menu* mn)
{
    for(int i = 0; i < mn->spriteCount; i++)
        spr_destroySprite(mn->sprites[i]);
    free(mn->sprites);
    mn->sprites = NULL;
    
    for(int i = 0; i < mn->buttonCount; i++)
        btn_destroyButton(mn->buttons[i]);
    free(mn->buttons);
    mn->buttons = NULL;

    for(int i = 0; i < mn->eventListenerCount; i++)
        evl_destroyEventListener(mn->eventListeners[i]);
    free(mn->eventListeners);
    mn->eventListeners = NULL;

    bg_destroyBG(mn->bg);

    free(mn);
    mn = NULL;
}

static void mn_initOnEventObj(mn_Menu *mn, oe_OnEvent *oeObj, mn_OnEventCfg *oeCfg){
    switch(oeCfg->onEventType){
        default:
        case MN_ONEVENT_NONE:
            oeObj->fn = NULL;
            oeObj->argc = 0;
            oeObj->argv = NULL;
            break;
        case MN_ONEVENT_SWITCHMENU:
            oeObj->fn = &mn_switchMenu;
            oeObj->argc = 2;
            oeObj->argv = malloc(sizeof(void*) * 2);
            oeObj->argv[0] = &mn->switchTo;
            oeObj->argv[1] = (void*)oeCfg->onEventArgs.switchToID;
            break;
        case MN_ONEVENT_GOBACK:
            oeObj->fn = &mn_goBack;
            oeObj->argc = 1;
            oeObj->argv = malloc(sizeof(void*) * 1);
            oeObj->argv[0] = &mn->statusFlags;
            break;
        case MN_ONEVENT_MEMCOPY:
            oeObj->fn = &mn_memCopy;
            oeObj->argc = 3;
            oeObj->argv = malloc(sizeof(void*) * 3);
            oeObj->argv[0] = oeCfg->onEventArgs.memCopy.source;
            oeObj->argv[1] = oeCfg->onEventArgs.memCopy.dest;
            oeObj->argv[2] = (void*)oeCfg->onEventArgs.memCopy.size;
            break;
        case MN_ONEVENT_PRINTMESSAGE:
            oeObj->fn = &mn_printMessage;
            oeObj->argc = 1;
            oeObj->argv = malloc(sizeof(void*) * 1);
            oeObj->argv[0] = (void*)oeCfg->onEventArgs.message;
            break;
        case MN_ONEVENT_EXECUTE_OTHER:
            oeObj->fn = &mn_executeOther;
            oeObj->argc = 1;
            oeObj->argv = malloc(sizeof(void*) * 1);
            oeObj->argv[0] = (void*)oeCfg->onEventArgs.executeOther;
            break;
    }
}

static int mn_switchMenu(int argc, void **argv)
{
    if(argc != 2 || argv[0] == NULL)
        return EXIT_FAILURE;

    unsigned long *menuIDPtr = argv[0];
    unsigned long switchTo = (mn_ID)argv[1];
    if(!menuIDPtr)
        return EXIT_FAILURE;

    *menuIDPtr = switchTo;

    return EXIT_SUCCESS;
}

static int mn_goBack(int argc, void **argv)
{
    if(argc != 1 || !argv[0])
        return EXIT_FAILURE;

    unsigned long *statusFlagsPtr = (unsigned long*)argv[0];
    *statusFlagsPtr |= MN_STATUS_GOBACK;
    return EXIT_SUCCESS;
}

static int mn_memCopy(int argc, void **argv)
{
    if(argc != 3 || argv[0] == NULL || argv[1] == NULL || argv[2] == NULL)
        return EXIT_FAILURE;
    
    const void *source = argv[0];
    void *dest = argv[1];
    size_t size = (size_t)argv[2];
    if(memcpy(dest, source, size) == NULL)
        return EXIT_FAILURE;
    else
        return EXIT_SUCCESS;
}

static int mn_printMessage(int argc, void **argv)
{
    if(argc != 1 || !argv[0])
        return EXIT_FAILURE;

    printf("%s", (const char*)argv[0]);
    return EXIT_SUCCESS;
}

static int mn_executeOther(int argc, void **argv)
{
    if(argc != 1 || !argv[0])
        return EXIT_FAILURE;

    /* Execute the function that argv[0] points to */
    ((void(*)())argv[0])();
    return EXIT_FAILURE;
}
