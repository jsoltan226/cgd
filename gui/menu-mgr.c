#include <cgd/gui/menu-mgr.h>
#include <cgd/gui/event-listener.h>
#include <cgd/gui/menu.h>
#include <cgd/gui/on-event.h>
#include <SDL2/SDL_render.h>
#include <stdlib.h>
#include <assert.h>
#include <error.h>

mmgr_MenuManager* mmgr_initMenuManager(mmgr_MenuManagerConfig* cfg, SDL_Renderer* renderer, kb_Keyboard *keyboard, ms_Mouse *mouse)
{
    /* Allocate space for the menu manager struct */
    mmgr_MenuManager* mmgr = malloc(sizeof(mmgr_MenuManager));
    assert(mmgr != NULL);
    
    /* Copy over the counts of the menus and global event listeners */
    mmgr->menuCount = cfg->menuCount;
    mmgr->globalEventListenerCount = cfg->globalEventListenerCount;

    /* Allocate space for the menus and global event listeners */
    mmgr->fullMenuList = malloc(cfg->menuCount * sizeof(mn_Menu*));
    mmgr->globalEventListeners = malloc(cfg->globalEventListenerCount * sizeof(evl_EventListener*));
    assert(
            (mmgr->fullMenuList != NULL || cfg->menuCount == 0) && 
            (mmgr->globalEventListeners != NULL || cfg->globalEventListenerCount == 0)
          );

    /* Set other members to default values */
    mmgr->inMenuDepth = 0;
    mmgr->previousMenus = NULL;

    /* Initialize the menus */
    for(int i = 0; i < cfg->menuCount; i++){
        mmgr->fullMenuList[i] = mn_initMenu(&cfg->menus[i], renderer, keyboard, mouse);
        assert(mmgr->fullMenuList[i] != NULL);
    }

    /* Initialize the global event listeners */
    evl_Target tempEvlTargetObj = {
        .keyboard = keyboard,
        .mouse = mouse,
    };
    for(int i = 0; i < cfg->globalEventListenerCount; i++){
        /* Use the mn_OnEvent interface for initializing oe_OnEvent structs */
        oe_OnEvent tempOEObj;
        mn_initOnEventObj(&tempOEObj, &cfg->globalEventListenerOnEventCfgs[i], NULL);

        /* Initialize the event listener */
        mmgr->globalEventListeners[i] = evl_initEventListener(&cfg->globalEventListenerCfgs[i], &tempOEObj, &tempEvlTargetObj);
        assert(mmgr->globalEventListeners[i] != NULL);
    }

    /* Set the current menu to be the first menu on the list */
    mmgr->currentMenu = mmgr->fullMenuList[0];

    return mmgr;
}

void mmgr_updateMenuManager(mmgr_MenuManager* mmgr, kb_Keyboard *keyboard, ms_Mouse *mouse, bool paused)
{
    /* Update the event listeners first */
    for(int i = 0; i < mmgr->globalEventListenerCount; i++){
        evl_updateEventListener(mmgr->globalEventListeners[i]);
    }

    /* If paused, only update the event listeners. This is a temporary solution, though */
    if(paused) return;

    /* Update the menus */
    mn_updateMenu(mmgr->currentMenu, mouse);

    /* Switch the menu if it has its 'switchTo' member set */
    if(mmgr->currentMenu->switchTo != MN_ID_NULL){
        mmgr_switchMenu(mmgr, mmgr->currentMenu->switchTo);
        ms_forceReleaseMouse(mouse, MS_EVERYBUTTONMASK);
    }
    /* Go back 1 menu if the current members' MN_ONEVENT_GOBACK status flag is set */
    if(mmgr->currentMenu->statusFlags & MN_ONEVENT_GOBACK){
        mmgr->currentMenu->statusFlags ^= MN_ONEVENT_GOBACK;
        mmgr_goBackMenu(mmgr);
    }
}

void mmgr_drawMenuManager(mmgr_MenuManager* mmgr, SDL_Renderer* renderer)
{
    /* Event listeners don't need drawing */

    mn_drawMenu(mmgr->currentMenu, renderer);
}

void mmgr_destroyMenuManager(mmgr_MenuManager* mmgr)
{
    /* Destroy the menus held in the full menu list, and the the list itself */
    for(int i = 0; i < mmgr->menuCount; i++)
    {
        mn_destroyMenu(mmgr->fullMenuList[i]);
    }
    free(mmgr->fullMenuList);
    mmgr->fullMenuList = NULL;

    /* Clean up other allocated arrays */
    free(mmgr->previousMenus);
    mmgr->previousMenus = NULL;

    /* Destroy the event listeners and the array olding them */
    for(int i = 0; i < mmgr->globalEventListenerCount; i++)
    {
        evl_destroyEventListener(mmgr->globalEventListeners[i]);
    }
    free(mmgr->globalEventListeners);
    mmgr->globalEventListeners = NULL;

    /* And finally, free the struct itself */
    free(mmgr);
    mmgr = NULL;
}

void mmgr_goBackMenu(mmgr_MenuManager* mmgr)
{
    /* If there are no previous menus to go back to, don't do anything */
    if(mmgr->inMenuDepth <= 0)
        return;
    else {
        /* Pop the previousMenus array into the currentMenu */
        mmgr->currentMenu = mmgr->previousMenus[mmgr->inMenuDepth - 1];

        /* Update the mmgr->inMenuDepth member and size of the previousMenus list accordingly */
        mmgr->inMenuDepth--;
        mmgr->previousMenus = realloc(mmgr->previousMenus, mmgr->inMenuDepth * sizeof(mn_Menu*));
        assert(mmgr->previousMenus != NULL);
    }
}

void mmgr_switchMenu(mmgr_MenuManager* mmgr, mn_ID switchTo)
{
    /* Check if a menu with the given ID exists in the menu list. If it doesn't, don't do anything. */
    mn_Menu *destMenuPtr = NULL;
    for(int i = 0; i < mmgr->menuCount; i++){
        if(mmgr->fullMenuList[i]->ID == switchTo){
            destMenuPtr = mmgr->fullMenuList[i];
            break;
        }
    }

    if(destMenuPtr){
        /* Reset the current menus' switchTo member */
        mmgr->currentMenu->switchTo = MN_ID_NULL;

        /* Update the mmgr->previousMenus array and push the current menu to the top of it */
        mmgr->previousMenus = realloc(mmgr->previousMenus, (mmgr->inMenuDepth + 1) * sizeof(mn_Menu*));
        assert(mmgr->previousMenus != NULL);

        mmgr->previousMenus[mmgr->inMenuDepth] = mmgr->currentMenu;

        /* Update the currentMenu pointer and the inMenuDepth accordingly */
        mmgr->currentMenu = destMenuPtr;
        mmgr->inMenuDepth++;
    } 
}
