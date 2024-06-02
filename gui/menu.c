#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include "menu.h"
#include <cgd/gui/buttons.h>
#include <cgd/gui/on-event.h>
#include <cgd/gui/parallax-bg.h>
#include <cgd/gui/sprite.h>

/* Here are the declarations of internal mn_OnEvent interface functions. Please see the definitions at the end of this file for detailed explanations of what they do */
static int mn_memCopy(int argc, void **argv);
static int mn_switchMenu(int argc, void **argv);
static int mn_goBack(int argc, void **argv);
static int mn_printMessage(int argc, void **argv);
static int mn_flipBool(int argc, void **argv);
#ifndef CGD_BUILDTYPE_RELEASE
static int mn_executeOther(int argc, void **argv);
#endif /* CGD_BUILDTYPE_RELEASE */

mn_Menu* mn_initMenu(mn_MenuConfig* cfg, SDL_Renderer* renderer,
    struct keyboard *keyboard, struct mouse *mouse)
{

    /* Memory allocation section */
    mn_Menu* mn = calloc(1, sizeof(mn_Menu));
    assert(mn != NULL);

    mn->sprites.ptrArray = malloc(sizeof(spr_Sprite) * cfg->spriteInfo.count);
    mn->buttons.ptrArray = malloc(sizeof(btn_Button) * cfg->buttonInfo.count);
    mn->eventListeners.ptrArray = malloc(sizeof(evl_EventListener) * cfg->eventListenerInfo.count);
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
    evl_Target tempEvlTargetObj = {
        .keyboard = keyboard,
        .mouse = mouse,
    };
    for(int i = 0; i < cfg->eventListenerInfo.count; i++){
        mn_eventListenerConfig *currentCfg = &cfg->eventListenerInfo.cfgs[i];
        oe_OnEvent onEventObj;
        mn_initOnEventObj(&onEventObj, &currentCfg->onEventCfg, mn);
        mn->eventListeners.ptrArray[i] = 
            evl_initEventListener(&currentCfg->eventListenerCfg, &onEventObj, &tempEvlTargetObj);
    }

    for(int i = 0; i < cfg->spriteInfo.count; i++){
        mn->sprites.ptrArray[i] = spr_initSprite(&cfg->spriteInfo.cfgs[i], renderer);
    }

    for(int i = 0; i < cfg->buttonInfo.count; i++){
        oe_OnEvent onClickObj;
        mn_initOnEventObj(&onClickObj, &cfg->buttonInfo.cfgs[i].onClickCfg, mn);
        mn->buttons.ptrArray[i] = btn_initButton(
            &cfg->buttonInfo.cfgs[i].spriteCfg,
            &onClickObj,
            cfg->buttonInfo.cfgs[i].flags,
            renderer
        );
    }

    mn->bg = bg_initBG(&cfg->bgConfig, renderer);

    return mn;
}

void mn_updateMenu(mn_Menu* mn, struct mouse *mouse)
{
    /* Sprites do not need updating; they never change */

    for(int i = 0; i < mn->buttons.count; i++){
        btn_updateButton(mn->buttons.ptrArray[i], mouse);
    }

    for(int i = 0; i < mn->eventListeners.count; i++){
        evl_updateEventListener(mn->eventListeners.ptrArray[i]);
    }

    bg_updateBG(mn->bg);
}

void mn_drawMenu(mn_Menu* mn, SDL_Renderer* renderer)
{
    /* Draw background below everything else */
    bg_drawBG(mn->bg, renderer);

    for(int i = 0; i < mn->sprites.count; i++){
        spr_drawSprite(mn->sprites.ptrArray[i], renderer);
    }

    for(int i = 0; i < mn->buttons.count; i++){
        btn_drawButton(mn->buttons.ptrArray[i], renderer);
    }

    /* Event listeners do not need drawing because they are invisible */
}

void mn_destroyMenu(mn_Menu* mn)
{
    for(int i = 0; i < mn->sprites.count; i++)
        spr_destroySprite(mn->sprites.ptrArray[i]);
    free(mn->sprites.ptrArray);
    mn->sprites.ptrArray = NULL;
     
    for(int i = 0; i < mn->buttons.count; i++)
        btn_destroyButton(mn->buttons.ptrArray[i]);
    free(mn->buttons.ptrArray);
    mn->buttons.ptrArray = NULL; 

    for(int i = 0; i < mn->eventListeners.count; i++)
        evl_destroyEventListener(mn->eventListeners.ptrArray[i]);
    free(mn->eventListeners.ptrArray);
    mn->eventListeners.ptrArray = NULL;

    bg_destroyBG(mn->bg);

    free(mn);
    mn = NULL;
}

void mn_initOnEventObj(oe_OnEvent *oeObj, mn_OnEventConfig *oeCfg, mn_Menu *optionalMenuPtr)
{
    /* =====> Do not bother reading this, see explanations in the menu.h header file  <===== */
    memset(oeObj->argv, 0, sizeof(void*) * OE_ARGV_SIZE);
    switch(oeCfg->onEventType){
        default:
        case MN_ONEVENT_NONE:
            oeObj->fn = NULL;
            oeObj->argc = 0;
            break;
        case MN_ONEVENT_SWITCHMENU:
            assert(optionalMenuPtr != NULL);
            oeObj->fn = &mn_switchMenu;
            oeObj->argc = 2;
            oeObj->argv[0] = &optionalMenuPtr->switchTo;
            oeObj->argv[1] = (void*)oeCfg->onEventArgs.switchDestMenuID;
            break;
        case MN_ONEVENT_QUIT: case MN_ONEVENT_PAUSE:
        case MN_ONEVENT_FLIPBOOL:
            oeObj->fn = &mn_flipBool;
            oeObj->argc = 1;
            oeObj->argv[0] = (void*)oeCfg->onEventArgs.boolVarPtr;
            break;
        case MN_ONEVENT_GOBACK:
            assert(optionalMenuPtr != NULL);
            oeObj->fn = &mn_goBack;
            oeObj->argc = 1;
            oeObj->argv[0] = (void*)&optionalMenuPtr->statusFlags;
            break;
        case MN_ONEVENT_MEMCOPY:
            oeObj->fn = &mn_memCopy;
            oeObj->argc = 3;
            oeObj->argv[0] = oeCfg->onEventArgs.memCopyInfo.source;
            oeObj->argv[1] = oeCfg->onEventArgs.memCopyInfo.dest;
            oeObj->argv[2] = (void*)oeCfg->onEventArgs.memCopyInfo.size;
            break;
        case MN_ONEVENT_PRINTMESSAGE:
            oeObj->fn = &mn_printMessage;
            oeObj->argc = 1;
            oeObj->argv[0] = (void*)oeCfg->onEventArgs.message;
            break;
        case MN_ONEVENT_EXECUTE_OTHER:
            oeObj->fn = &mn_executeOther;
            oeObj->argc = 1;
            oeObj->argv[0] = (void*)oeCfg->onEventArgs.executeOther;
            break;
    }
}

/* =====> Here are the internal menu.c functions used for performing the various operations <======
 * =====> selected with the mn_OnEvent interface                                            <====== */
static int mn_switchMenu(int argc, void **argv)
{
    /* check for argument existence and validity */
    if(!argv)
        return EXIT_FAILURE;

    if(argc != 2 || argv[0] == NULL)
        return EXIT_FAILURE;


    /* argv[0] contains a pointer to an mn->switchTo variable, which we will need to change to whatever argv[1] is */
    mn_ID *menuIDPtr = (mn_ID*)argv[0];

    /* argv[1] contains the direct value of the id of the menu we want to switch to */
    mn_ID switchTo = (mn_ID)argv[1];
    if(!menuIDPtr)
        return EXIT_FAILURE;

    *menuIDPtr = switchTo;

    return EXIT_SUCCESS;
}

static int mn_goBack(int argc, void **argv)
{
    /* check for argument existence and validity */
    if(!argv)
        return EXIT_FAILURE;

    if(argc != 1 || !argv[0])
        return EXIT_FAILURE;

    /* argv[0] contains the pointer to a mn->statusFlags var, so first cast it to the appropriate type,
      dereference it, and set the bit on MN_STATUS_GOBACK to 1 using the bitwise or (|=) operation */
    unsigned long *statusFlagsPtr = (unsigned long*)argv[0];
    *statusFlagsPtr |= MN_STATUS_GOBACK;
    return EXIT_SUCCESS;
}

static int mn_memCopy(int argc, void **argv)
{
    /* =====> AVOID USING THIS FUNCTION. IT IS NOT VERY SAFE. <===== */
    /* check for argument existence and validity */
    if(!argv)
        return EXIT_FAILURE;

    if(argc != 3 || argv[0] == NULL || argv[1] == NULL || argv[2] == NULL)
        return EXIT_FAILURE;
    
    const void *source = argv[0];
    void *dest = argv[1];
    /*
     * We are hiding a size_t direct value in argv[2], which is of type void*,
     * so we need to trick the compiler to net producing an error by first casting it to type size_t*,
     * and then dereferencing it.
    */
    size_t size = *(size_t*)argv[2];
    if(memcpy(dest, source, size) == NULL)
        return EXIT_FAILURE;
    else
        return EXIT_SUCCESS;
}

static int mn_printMessage(int argc, void **argv)
{
    /* check for argument existence and validity */
    if(!argv)
        return EXIT_FAILURE;

    if(argc != 1 || !argv[0])
        return EXIT_FAILURE;

    /* here argv[0] contains the pointer to the string we want to print */
    printf("%s", (const char*)argv[0]);
    return EXIT_SUCCESS;
}

static int mn_flipBool(int argc, void **argv)
{
    /* check for argument existence and validity */
    if(!argv)
        return EXIT_FAILURE;

    if(argc != 1 || !argv[0])
        return EXIT_FAILURE;
    
    /* argv[0] contains the pointer to a boolean variable, so convert it to a bool* and then dereference it */
    *(bool*)argv[0] = !(*(bool*)argv[0]);

    return EXIT_SUCCESS;
}

#ifndef CGD_BUILDDTYPE_RELEASE
static int mn_executeOther(int argc, void **argv)
{
    /* =====> THIS IS A VERY UNSAFE FUNCTION, USED FOR TESTING PURPOSES ONLY! DO NOT USE IT IN PRODUCTION CODE!!! <===== */

    /* check for argument existence and validity */
    if(!argv)
        return EXIT_FAILURE;

    if(argc != 1 || !argv[0])
        return EXIT_FAILURE;

    /* Execute the function that argv[0] points to */
    ( ( void(*)() )argv[0] ) ();

    /*
     * Argv[0] contains the pointer to the function we want to execute
     * So first, we cast argv[0] to the function pointer type that we need - no arguments, returns nothing
     * ( `void(*)()` )
     *     here =>  ( ( void(*)() )argv[0] ) <=  .......
     *
     * Then we execute the casted argv[0] as if it were a normal function
     *    (cast argv[0] ...) => () <= here
    */
    return EXIT_FAILURE;
}
#endif /* CGD_BUILDTYPE_RELEASE */
