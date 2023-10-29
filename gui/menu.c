#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include "event-listener.h"
#include "menu.h"
#include "buttons.h"
#include "parallax-bg.h"
#include "sprite.h"

static int mn_memCopy(int argc, void **argv);
static int mn_switchMenu(int argc, void **argv);
static int mn_printMessage(int argc, void **argv);
static int mn_executeOther(int argc, void **argv);

mn_Menu* mn_initMenu(mn_MenuConfig* cfg, SDL_Renderer* renderer, input_Keyboard keyboard, input_Mouse *mouse)
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
        mn->eventListeners[i] = evl_initEventListener(t, &cfg->eventListenerCfgs[i]);
    }

    for(int i = 0; i < cfg->spriteCount; i++){
        mn->sprites[i] = spr_initSprite(&cfg->spriteCfgs[i], renderer);
    }

    for(int i = 0; i < cfg->buttonCount; i++){
        btn_OnClickConfig onClickCfg;
        switch(cfg->buttonOnClickCfgs[i].onClickType){
            default:
            case MN_ONCLICK_NONE:
                onClickCfg.fn = NULL;
                onClickCfg.argc = 0;
                onClickCfg.argv = NULL;
                break;
            case MN_ONCLICK_SWITCHMENU:
                onClickCfg.fn = &mn_switchMenu;
                onClickCfg.argc = 2;
                onClickCfg.argv = malloc(sizeof(void*) * 2);
                onClickCfg.argv[0] = &mn->switchTo;
                onClickCfg.argv[1] = (void*)cfg->buttonOnClickCfgs[i].onClickArgs.switchToID;
                break;
            case MN_ONCLICK_MEMCOPY:
                onClickCfg.fn = &mn_memCopy;
                onClickCfg.argc = 3;
                onClickCfg.argv = malloc(sizeof(void*) * 3);
                onClickCfg.argv[0] = cfg->buttonOnClickCfgs[i].onClickArgs.memCopy.source;
                onClickCfg.argv[1] = cfg->buttonOnClickCfgs[i].onClickArgs.memCopy.dest;
                onClickCfg.argv[2] = (void*)cfg->buttonOnClickCfgs[i].onClickArgs.memCopy.size;
                break;
            case MN_ONCLICK_PRINTMESSAGE:
                onClickCfg.fn = &mn_printMessage;
                onClickCfg.argc = 1;
                onClickCfg.argv = malloc(sizeof(void*) * 1);
                onClickCfg.argv[0] = (void*)cfg->buttonOnClickCfgs[i].onClickArgs.message;
                break;
            case MN_ONCLICK_EXECUTE_OTHER:
                onClickCfg.fn = &mn_executeOther;
                onClickCfg.argc = 1;
                onClickCfg.argv = malloc(sizeof(void*) * 1);
                onClickCfg.argv[0] = (void*)cfg->buttonOnClickCfgs[i].onClickArgs.executeOther;
                break;
        }
        mn->buttons[i] = btn_initButton(&cfg->buttonSpriteCfgs[i], &onClickCfg, renderer);
    }

    mn->bg = bg_initBG(&cfg->bgConfig, renderer);

    return mn;
}

void mn_updateMenu(mn_Menu* mn, input_Mouse *mouse)
{
    for(int i = 0; i < mn->buttonCount; i++){
        btn_updateButton(mn->buttons[i], mouse);
    }
    for(int i = 0; i < mn->eventListenerCount; i++){
        evl_updateEventListener(mn->eventListeners[i]);
        if(mn->eventListeners[i]->detected)
            puts("Event listener detected event!");
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
    
    for(int i = 0; i < mn->buttonCount; i++){
        free(mn->buttons[i]->onClickArgv);
        mn->buttons[i]->onClickArgv = NULL;
        btn_destroyButton(mn->buttons[i]);
    }
    free(mn->buttons);
    mn->buttons = NULL;

    for(int i = 0; i < mn->eventListenerCount; i++){
        evl_destroyEventListener(mn->eventListeners[i]);
    }
    free(mn->eventListeners);
    mn->eventListeners = NULL;

    bg_destroyBG(mn->bg);
    mn->bg = NULL;

    free(mn);
    mn = NULL;
}

static int mn_switchMenu(int argc, void **argv)
{
    if(argc != 2 || !argv[0])
        return EXIT_FAILURE;

    unsigned long *menuIDPtr = argv[0];
    unsigned long switchTo = (mn_ID)argv[1];
    *menuIDPtr = switchTo;

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
