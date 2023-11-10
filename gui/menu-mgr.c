#include "menu-mgr.h"
#include "event-listener.h"
#include "menu.h"
#include "on-event.h"
#include <SDL2/SDL_render.h>
#include <stdlib.h>
#include <assert.h>
#include <error.h>

mmgr_MenuManager* mmgr_initMenuManager(mmgr_MenuManagerConfig* cfg, SDL_Renderer* renderer, kb_Keyboard *keyboard, ms_Mouse *mouse)
{
    mmgr_MenuManager* mmgr = malloc(sizeof(mmgr_MenuManager));
    assert(mmgr != NULL);
    
    mmgr->menuCount = cfg->menuCount;
    mmgr->globalEventListenerCount = cfg->globalEventListenerCount;
    mmgr->fullMenuList = malloc(cfg->menuCount * sizeof(mn_Menu*));
    mmgr->previousMenus = malloc(0);
    mmgr->globalEventListeners = malloc(cfg->globalEventListenerCount * sizeof(evl_EventListener*));
    assert(mmgr->fullMenuList != NULL && mmgr->previousMenus != NULL && mmgr->globalEventListeners != NULL);

    mmgr->inMenuDepth = 0;

    for(int i = 0; i < cfg->menuCount; i++){
        mmgr->fullMenuList[i] = mn_initMenu(&cfg->menus[i], renderer, keyboard, mouse);
    }

    evl_Target tempEvlTargetObj = {
        .keyboard = keyboard,
        .mouse = mouse,
    };
    for(int i = 0; i < cfg->globalEventListenerCount; i++){
        oe_OnEvent tempOEObj;
        mn_initOnEventObj(&tempOEObj, &cfg->globalEventListenerOnEventCfgs[i], NULL);
        mmgr->globalEventListeners[i] = evl_initEventListener(&cfg->globalEventListenerCfgs[i], &tempOEObj, &tempEvlTargetObj);
    }

    mmgr->currentMenu = mmgr->fullMenuList[0];

    return mmgr;
}

void mmgr_updateMenuManager(mmgr_MenuManager* mmgr, kb_Keyboard *keyboard, ms_Mouse *mouse)
{
    for(int i = 0; i < mmgr->globalEventListenerCount; i++){
        evl_updateEventListener(mmgr->globalEventListeners[i]);
    }

    mn_updateMenu(mmgr->currentMenu, mouse);

    if(mmgr->currentMenu->switchTo != MN_ID_NULL){
        mmgr_switchMenu(mmgr, mmgr->currentMenu->switchTo);
        ms_forceReleaseMouse(mouse, MS_EVERYBUTTONMASK);
    }
    if(mmgr->currentMenu->statusFlags & MN_ONEVENT_GOBACK){
        mmgr->currentMenu->statusFlags ^= MN_ONEVENT_GOBACK;
        mmgr_goBackMenu(mmgr);
    }
}

void mmgr_drawMenuManager(mmgr_MenuManager* mmgr, SDL_Renderer* renderer, bool displayButtonHitboxes)
{
    mn_drawMenu(mmgr->currentMenu, renderer, displayButtonHitboxes);
}

void mmgr_destroyMenuManager(mmgr_MenuManager* mmgr)
{
    for(int i = 0; i < mmgr->menuCount; i++)
    {
        mn_destroyMenu(mmgr->fullMenuList[i]);
    }
    free(mmgr->fullMenuList);
    free(mmgr->previousMenus);
    mmgr->fullMenuList = NULL;
    mmgr->previousMenus = NULL;

    for(int i = 0; i < mmgr->globalEventListenerCount; i++)
    {
        evl_destroyEventListener(mmgr->globalEventListeners[i]);
    }
    free(mmgr->globalEventListeners);
    mmgr->globalEventListeners = NULL;

    free(mmgr);
    mmgr = NULL;
}

void mmgr_goBackMenu(mmgr_MenuManager* mmgr)
{
    if(mmgr->inMenuDepth <= 0)
        return;
    else {
        mmgr->currentMenu = mmgr->previousMenus[mmgr->inMenuDepth - 1];
        mmgr->currentMenu = mmgr->fullMenuList[0];
        mmgr->previousMenus = realloc(mmgr->previousMenus, (mmgr->inMenuDepth - 1) * sizeof(mn_Menu*));
        mmgr->inMenuDepth--;
    }
}

void mmgr_switchMenu(mmgr_MenuManager* mmgr, mn_ID switchTo)
{
    mn_Menu *destMenuPtr = NULL;
    for(int i = 0; i < mmgr->menuCount; i++){
        if(mmgr->fullMenuList[i]->ID == switchTo){
            destMenuPtr = mmgr->fullMenuList[i];
            break;
        }
    }
    if(destMenuPtr){
        mmgr->previousMenus = realloc(mmgr->previousMenus, (mmgr->inMenuDepth + 1) * sizeof(mn_Menu*));
        mmgr->previousMenus[mmgr->inMenuDepth] = mmgr->currentMenu;
        mmgr->currentMenu->switchTo = MN_ID_NULL;
        mmgr->currentMenu = destMenuPtr;
        mmgr->inMenuDepth++;
    } 
}
