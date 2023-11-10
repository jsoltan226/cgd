#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include "event-listener.h"
#include "menu.h"
#include "buttons.h"
#include "on-event.h"
#include "parallax-bg.h"
#include "sprite.h"

/* Here are the declarations of internal mn_OnEvent interface functions. Please see the definitions at the end of this file for detailed explanations of what they do */
static int mn_memCopy(int argc, void **argv);
static int mn_switchMenu(int argc, void **argv);
static int mn_goBack(int argc, void **argv);
static int mn_printMessage(int argc, void **argv);
static int mn_quit(int argc, void **argv);
static int mn_executeOther(int argc, void **argv);

mn_Menu* mn_initMenu(mn_MenuConfig* cfg, SDL_Renderer* renderer, kb_Keyboard *keyboard, ms_Mouse *mouse)
{
    mn_Menu* mn = malloc(sizeof(mn_Menu));
    assert(mn != NULL);

    /* Get the counts of all the dynamic array members' elements */
    mn->spriteCount = cfg->spriteCount;
    mn->buttonCount = cfg->buttonCount;
    mn->eventListenerCount = cfg->eventListenerCount;

    /* Allocate memory for all the members, check for failures */
    mn->sprites = malloc(sizeof(spr_Sprite) * cfg->spriteCount);
    mn->buttons = malloc(sizeof(btn_Button) * cfg->buttonCount);
    mn->eventListeners = malloc(sizeof(evl_EventListener) * cfg->eventListenerCount);
    assert( (mn->sprites != NULL || mn->spriteCount == 0) && 
            (mn->buttons != NULL || mn->buttonCount == 0) &&
            (mn->eventListeners != NULL || mn->eventListenerCount == 0)
          );

    /* Set other miscellaneous members to given values */
    mn->switchTo = MN_ID_NULL;
    mn->ID = cfg->id;
    mn->statusFlags = MN_STATUS_NONE;

    /* Initialize the event listeners */
    evl_Target tempEvlTargetObj = {
        .keyboard = keyboard,
        .mouse = mouse,
    };
    for(int i = 0; i < cfg->eventListenerCount; i++){
        oe_OnEvent onEventObj;
        mn_initOnEventObj(&onEventObj, &cfg->eventListenerOnEventCfgs[i], mn);
        mn->eventListeners[i] = evl_initEventListener(&cfg->eventListenerCfgs[i], &onEventObj, &tempEvlTargetObj);
    }

    /* Initialize the static sprites */
    for(int i = 0; i < cfg->spriteCount; i++){
        mn->sprites[i] = spr_initSprite(&cfg->spriteCfgs[i], renderer);
    }

    /* Initialize the buttons */
    for(int i = 0; i < cfg->buttonCount; i++){
        oe_OnEvent onClickObj;
        mn_initOnEventObj(&onClickObj, &cfg->buttonOnClickCfgs[i], mn);
        mn->buttons[i] = btn_initButton(&cfg->buttonSpriteCfgs[i], &onClickObj, renderer);
    }

    /* Initialize the background */
    mn->bg = bg_initBG(&cfg->bgConfig, renderer);

    return mn;
}

void mn_updateMenu(mn_Menu* mn, ms_Mouse *mouse)
{
    /* Sprites do not need updating; they never change */
    /* Update buttons */
    for(int i = 0; i < mn->buttonCount; i++){
        btn_updateButton(mn->buttons[i], mouse);
    }

    /* Update the event listeners */
    for(int i = 0; i < mn->eventListenerCount; i++){
        evl_updateEventListener(mn->eventListeners[i]);
    }

    /* Update the background */
    bg_updateBG(mn->bg);
}

void mn_drawMenu(mn_Menu* mn, SDL_Renderer* renderer, bool displayButtonHitboxes)
{
    /* Draw background below everything else */
    bg_drawBG(mn->bg, renderer);

    /* Draw the sprites */
    for(int i = 0; i < mn->spriteCount; i++){
        spr_drawSprite(mn->sprites[i], renderer);
    }

    /* Draw the buttons */
    for(int i = 0; i < mn->buttonCount; i++){
        btn_drawButton(mn->buttons[i], renderer, displayButtonHitboxes);
    }

    /* Event listeners do not need drawing because they are invisible */

}

void mn_destroyMenu(mn_Menu* mn)
{
    /* Destroy all the sprites */
    for(int i = 0; i < mn->spriteCount; i++)
        spr_destroySprite(mn->sprites[i]);
    free(mn->sprites);
    mn->sprites = NULL;
     
    /* Destroy the buttons */
    for(int i = 0; i < mn->buttonCount; i++)
        btn_destroyButton(mn->buttons[i]);
    free(mn->buttons);
    mn->buttons = NULL; 

    /* Destroy the event listeners */
    for(int i = 0; i < mn->eventListenerCount; i++)
        evl_destroyEventListener(mn->eventListeners[i]);
    free(mn->eventListeners);
    mn->eventListeners = NULL;

    /* Destroy the background */
    bg_destroyBG(mn->bg);

    /* And finally, free the menu struct itself */
    free(mn);
    mn = NULL;
}

void mn_initOnEventObj(oe_OnEvent *oeObj, mn_OnEventCfg *oeCfg, mn_Menu *optionalMenuPtr)
{
    /* =====> Do not bother reading this, see explanations in the menu.h header file  <===== */
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
            oeObj->argv[0] = &optionalMenuPtr->switchTo;
            oeObj->argv[1] = (void*)oeCfg->onEventArgs.switchDestMenuID;
            break;
        case MN_ONEVENT_QUIT:
            oeObj->fn = &mn_quit;
            oeObj->argc = 1;
            oeObj->argv = malloc(sizeof(void*) * 1);
            oeObj->argv[0] = (void*)oeCfg->onEventArgs.runningVarPtr;
            break;
        case MN_ONEVENT_GOBACK:
            oeObj->fn = &mn_goBack;
            oeObj->argc = 1;
            oeObj->argv = malloc(sizeof(void*) * 1);
            oeObj->argv[0] = (void*)&optionalMenuPtr->statusFlags;
            break;
        case MN_ONEVENT_MEMCOPY:
            oeObj->fn = &mn_memCopy;
            oeObj->argc = 3;
            oeObj->argv = malloc(sizeof(void*) * 3);
            oeObj->argv[0] = oeCfg->onEventArgs.memCopyInfo.source;
            oeObj->argv[1] = oeCfg->onEventArgs.memCopyInfo.dest;
            oeObj->argv[2] = (void*)oeCfg->onEventArgs.memCopyInfo.size;
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

static int mn_quit(int argc, void **argv)
{
    /* check for argument existence and validity */
    if(!argv)
        return EXIT_FAILURE;

    if(argc != 1 || !argv[0])
        return EXIT_FAILURE;
    
    /* argv[0] contains the pointer to a 'running' variable, so cast it to bool*, dereference it and set it to false */
    *(bool*)argv[0] = false;

    return EXIT_SUCCESS;
}

static int mn_executeOther(int argc, void **argv)
{
    /* =====> THIS IS A VERY UNSAFE FUNCTION, USED FOR TESTING PURPOSES ONLY! DO NOT USE IT IN PRODUCTION CODE!!! <===== */

    /* check for argument existence and validity */
    if(!argv)
        return EXIT_FAILURE;

    if(argc != 1 || !argv[0])
        return EXIT_FAILURE;

    /* Execute the function that argv[0] points to */
    ((void(*)())argv[0]) ();

    /*
     * Argv[0] contains the pointer to the function we want to execute
     * So first, we cast argv[0] to the function pointer type that we need - no arguments, returs void ( void(*)() )
     *     here =>  ((void(*)())argv[0]) <=  .......
     *
     * Then we execute the casted argv[0] as if it were a normal function
     *    (cast argv[0] ...) => () <= here
    */
    return EXIT_FAILURE;
}
