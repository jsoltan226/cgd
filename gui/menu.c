#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include "menu.h"
#include "event-listener.h"
#include <cgd/gui/buttons.h>
#include <cgd/gui/on-event.h>
#include <cgd/gui/parallax-bg.h>
#include <cgd/gui/sprite.h>

/* Here are the declarations of internal mn_OnEvent interface functions. Please see the definitions at the end of this file for detailed explanations of what they do */
static int mn_memCopy(u64 argv_buf[ONEVENT_OBJ_ARGV_LEN]);
static int mn_switchMenu(u64 argv_buf[ONEVENT_OBJ_ARGV_LEN]);
static int mn_goBack(u64 argv_buf[ONEVENT_OBJ_ARGV_LEN]);
static int mn_printMessage(u64 argv_buf[ONEVENT_OBJ_ARGV_LEN]);
static int mn_flipBool(u64 argv_buf[ONEVENT_OBJ_ARGV_LEN]);
#ifndef CGD_BUILDTYPE_RELEASE
static int mn_executeOther(u64 argv_buf[ONEVENT_OBJ_ARGV_LEN]);
#endif /* CGD_BUILDTYPE_RELEASE */

mn_Menu* mn_initMenu(mn_MenuConfig* cfg, SDL_Renderer* renderer,
    struct keyboard *keyboard, struct mouse *mouse)
{

    /* Memory allocation section */
    mn_Menu* mn = calloc(1, sizeof(mn_Menu));
    assert(mn != NULL);

    mn->sprites.ptrArray = malloc(sizeof(sprite_t) * cfg->spriteInfo.count);
    mn->buttons.ptrArray = malloc(sizeof(struct button) * cfg->buttonInfo.count);
    mn->eventListeners.ptrArray = malloc(sizeof(struct event_listener) * cfg->eventListenerInfo.count);
    assert( (mn->sprites.ptrArray != NULL || mn->sprites.count == 0) &&
            (mn->buttons.ptrArray != NULL || mn->buttons.count == 0) &&
            (mn->eventListeners.ptrArray != NULL || mn->eventListeners.count == 0)
          );

    /* Copy-over-data section */
    mn->sprites.count = cfg->spriteInfo.count;
    mn->buttons.count = cfg->buttonInfo.count;
    mn->eventListeners.count = cfg->eventListenerInfo.count;
    mn->switchTo = MN_ID_NULL;
    mn->ID = cfg->id;
    mn->statusFlags = MN_STATUS_NONE;

    /* Initialization section */
    struct event_listener_target tempEvlTargetObj = {
        .keyboard = keyboard,
        .mouse = mouse,
    };
    for(int i = 0; i < cfg->eventListenerInfo.count; i++){
        mn_eventListenerConfig *currentCfg = &cfg->eventListenerInfo.cfgs[i];
        struct on_event_obj onEventObj;
        mn_initOnEventObj(&onEventObj, &currentCfg->onEventCfg, mn);
        mn->eventListeners.ptrArray[i] =
            event_listener_init(&currentCfg->eventListenerCfg, &onEventObj, &tempEvlTargetObj);
    }

    for(int i = 0; i < cfg->spriteInfo.count; i++){
        mn->sprites.ptrArray[i] = sprite_init(&cfg->spriteInfo.cfgs[i], renderer);
    }

    for(int i = 0; i < cfg->buttonInfo.count; i++){
        struct on_event_obj onClickObj;
        mn_initOnEventObj(&onClickObj, &cfg->buttonInfo.cfgs[i].onClickCfg, mn);
        mn->buttons.ptrArray[i] = button_init(
            &cfg->buttonInfo.cfgs[i].spriteCfg,
            &onClickObj,
            cfg->buttonInfo.cfgs[i].flags,
            renderer
        );
    }

    mn->bg = parallax_bg_init(&cfg->bgConfig, renderer);

    return mn;
}

void mn_updateMenu(mn_Menu* mn, struct mouse *mouse)
{
    /* Sprites do not need updating; they never change */

    for(int i = 0; i < mn->buttons.count; i++)
        button_update(mn->buttons.ptrArray[i], mouse);

    for(int i = 0; i < mn->eventListeners.count; i++)
        event_listener_update(mn->eventListeners.ptrArray[i]);

    parallax_bg_update(mn->bg);
}

void mn_drawMenu(mn_Menu* mn, SDL_Renderer* renderer)
{
    /* Draw background below everything else */
    parallax_bg_draw(mn->bg, renderer);

    for(int i = 0; i < mn->sprites.count; i++)
        sprite_draw(mn->sprites.ptrArray[i], renderer);

    for(int i = 0; i < mn->buttons.count; i++)
        button_draw(mn->buttons.ptrArray[i], renderer);

    /* Event listeners do not need drawing because they are invisible */
}

void mn_destroyMenu(mn_Menu* mn)
{
    for(int i = 0; i < mn->sprites.count; i++)
        sprite_destroy(mn->sprites.ptrArray[i]);
    free(mn->sprites.ptrArray);
    mn->sprites.ptrArray = NULL;

    for(int i = 0; i < mn->buttons.count; i++)
        button_destroy(mn->buttons.ptrArray[i]);
    free(mn->buttons.ptrArray);
    mn->buttons.ptrArray = NULL;

    for(int i = 0; i < mn->eventListeners.count; i++)
        event_listener_destroy(mn->eventListeners.ptrArray[i]);
    free(mn->eventListeners.ptrArray);
    mn->eventListeners.ptrArray = NULL;

    parallax_bg_destroy(mn->bg);

    free(mn);
}

void mn_initOnEventObj(struct on_event_obj *oeObj, mn_OnEventConfig *oeCfg, mn_Menu *optionalMenuPtr)
{
    /* =====> Do not bother reading this, see explanations in the menu.h header file  <===== */
    memset(oeObj->argv_buf, 0, ONEVENT_OBJ_ARGV_SIZE);
    switch(oeCfg->onEventType){
        default:
        case MN_ONEVENT_NONE:
            oeObj->fn = NULL;
            break;
        case MN_ONEVENT_SWITCHMENU:
            assert(optionalMenuPtr != NULL);
            oeObj->fn = &mn_switchMenu;
            oeObj->argv_buf[0] = (u64)(void *)&optionalMenuPtr->switchTo;
            oeObj->argv_buf[1] = (u64)(void *)oeCfg->onEventArgs.switchDestMenuID;
            break;
        case MN_ONEVENT_QUIT: case MN_ONEVENT_PAUSE:
        case MN_ONEVENT_FLIPBOOL:
            oeObj->fn = &mn_flipBool;
            oeObj->argv_buf[0] = (u64)(void *)oeCfg->onEventArgs.boolVarPtr;
            break;
        case MN_ONEVENT_GOBACK:
            assert(optionalMenuPtr != NULL);
            oeObj->fn = &mn_goBack;
            oeObj->argv_buf[0] = (u64)(void *)&optionalMenuPtr->statusFlags;
            break;
        case MN_ONEVENT_MEMCOPY:
            oeObj->fn = &mn_memCopy;
            oeObj->argv_buf[0] = (u64)(void *)oeCfg->onEventArgs.memCopyInfo.source;
            oeObj->argv_buf[1] = (u64)(void *)oeCfg->onEventArgs.memCopyInfo.dest;
            oeObj->argv_buf[2] = (u64)(void *)oeCfg->onEventArgs.memCopyInfo.size;
            break;
        case MN_ONEVENT_PRINTMESSAGE:
            oeObj->fn = &mn_printMessage;
            oeObj->argv_buf[0] = (u64)(void *)oeCfg->onEventArgs.message;
            break;
        case MN_ONEVENT_EXECUTE_OTHER:
            oeObj->fn = &mn_executeOther;
            oeObj->argv_buf[0] = (u64)(void *)oeCfg->onEventArgs.executeOther;
            break;
    }
}

/* =====> Here are the internal menu.c functions used for performing the various operations <======
 * =====> selected with the mn_OnEvent interface                                            <====== */
static int mn_switchMenu(u64 argv_buf[ONEVENT_OBJ_ARGV_LEN])
{
    /* check for argument existence and validity */
    if(argv_buf == NULL || (void *)argv_buf[0] == NULL)
        return EXIT_FAILURE;

    /* argv[0] contains a pointer to an mn->switchTo variable, which we will need to change to whatever argv[1] is */
    mn_ID *menuIDPtr = (void *)argv_buf[0];

    /* argv[1] contains the direct value of the id of the menu we want to switch to */
    mn_ID switchTo = argv_buf[1];
    if(menuIDPtr == NULL)
        return EXIT_FAILURE;

    *menuIDPtr = switchTo;

    return EXIT_SUCCESS;
}

static int mn_goBack(u64 argv_buf[ONEVENT_OBJ_ARGV_LEN])
{
    /* check for argument existence and validity */
    if(argv_buf == NULL || (void *)argv_buf[0] == NULL)
        return EXIT_FAILURE;

    /* argv[0] contains the pointer to a mn->statusFlags var, so first cast it to the appropriate type,
      dereference it, and set the bit on MN_STATUS_GOBACK to 1 using the bitwise or (|=) operation */
    u64 *statusFlagsPtr = (void *)argv_buf[0];
    *statusFlagsPtr |= MN_STATUS_GOBACK;
    return EXIT_SUCCESS;
}

static int mn_memCopy(u64 argv_buf[ONEVENT_OBJ_ARGV_LEN])
{
    /* =====> AVOID USING THIS FUNCTION. IT IS NOT VERY SAFE. <===== */
    /* check for argument existence and validity */

    if(argv_buf == NULL || 
        (void *)argv_buf[0] == NULL ||
        (void *)argv_buf[1] == NULL ||
        (void *)argv_buf[2] == NULL
    ) return EXIT_FAILURE;

    const void *source = (void *)argv_buf[0];
    void *dest = (void *)argv_buf[1];
    /*
     * We are hiding a size_t direct value in argv[2], which is of type void*,
     * so we need to trick the compiler to net producing an error by first casting it to type size_t*,
     * and then dereferencing it.
    */
    size_t size = *(size_t *)argv_buf[2];
    if(memcpy(dest, source, size) == NULL)
        return EXIT_FAILURE;
    else
        return EXIT_SUCCESS;
}

static int mn_printMessage(u64 argv_buf[ONEVENT_OBJ_ARGV_LEN])
{
    /* check for argument existence and validity */
    if(argv_buf == NULL || (void *)argv_buf[0] == NULL)
        return EXIT_FAILURE;

    /* here argv[0] contains the pointer to the string we want to print */
    printf("%s", (const char*)argv_buf[0]);
    return EXIT_SUCCESS;
}

static int mn_flipBool(u64 argv_buf[ONEVENT_OBJ_ARGV_LEN])
{
    /* check for argument existence and validity */
    if(argv_buf == NULL || (void *)argv_buf[0] == NULL)
        return EXIT_FAILURE;

    /* argv[0] contains the pointer to a boolean variable, so convert it to a bool* and then dereference it */
    *(bool *)argv_buf[0] = !(*(bool *)argv_buf[0]);

    return EXIT_SUCCESS;
}

#ifndef CGD_BUILDDTYPE_RELEASE
static int mn_executeOther(u64 argv_buf[ONEVENT_OBJ_ARGV_LEN])
{
    /* =====> THIS IS A VERY UNSAFE FUNCTION, USED FOR TESTING PURPOSES ONLY! DO NOT USE IT IN PRODUCTION CODE!!! <===== */

    /* check for argument existence and validity */
    if(argv_buf == NULL || (void *)argv_buf[0] == NULL)
        return EXIT_FAILURE;

    /* Execute the function that argv[0] points to */
    ( ( void(*)() )argv_buf[0] ) ();

    return EXIT_SUCCESS;
}
#endif /* CGD_BUILDTYPE_RELEASE */
