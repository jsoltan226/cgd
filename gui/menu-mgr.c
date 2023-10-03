#include "menu-mgr.h"
#include "menu.h"
#include <SDL2/SDL_render.h>
#include <stdlib.h>
#include <assert.h>
#include <error.h>

mmgr_MenuManager* mmgr_initMenuManager(mmgr_MenuManagerConfig* cfg, SDL_Renderer* renderer)
{
    assert(cfg->menuCount >= 0);

    mmgr_MenuManager* mmgr = malloc(sizeof(mmgr_MenuManager));
    assert(mmgr != NULL);
    
    mmgr->menuCount = cfg->menuCount;
    mmgr->fullMenuList = malloc(cfg->menuCount * sizeof(mn_Menu*));
    mmgr->previousMenus = malloc(0);
    assert(mmgr->fullMenuList != NULL && mmgr->previousMenus != NULL);

    mmgr->inMenuDepth = 0;

    for(int i = 0; i < cfg->menuCount; i++){
        mmgr->fullMenuList[i] = mn_initMenu(&cfg->menus[i], renderer);
    }

    mmgr->currentMenu = mmgr->fullMenuList[0];

    return mmgr;
}

void mmgr_updateMenuManager(mmgr_MenuManager* mmgr, input_Keyboard keyboard, input_Mouse mouse)
{
    mn_updateMenu(mmgr->currentMenu, mouse);

    if(mmgr->currentMenu->switchTo != NULL){
        mn_Menu* switchTo = mmgr->currentMenu->switchTo;
        mmgr->currentMenu->switchTo = NULL;
        mmgr_switchMenu(mmgr, switchTo);
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

void mmgr_switchMenu(mmgr_MenuManager* mmgr, mn_Menu* newMenu)
{
    bool createNewEntry = true;
    for(int i = 0; i < mmgr->menuCount; i++){
        if(newMenu == mmgr->fullMenuList[i]){
            createNewEntry = false;
            break;
        }
    }
    if(createNewEntry){
        mmgr->fullMenuList = realloc(mmgr->fullMenuList, (mmgr->menuCount + 1) * sizeof(mn_Menu*));
        mmgr->fullMenuList[mmgr->menuCount] = newMenu;
        mmgr->menuCount++;
    }

    if(mmgr->inMenuDepth < 0)
    {
        mmgr->menuCount = 0;
        mmgr->previousMenus = NULL;
        mmgr->currentMenu = newMenu;
        mmgr->inMenuDepth = 0;
        return;
    }

    mmgr->previousMenus = realloc(mmgr->previousMenus, (mmgr->inMenuDepth + 1) * sizeof(mn_Menu*));
    mmgr->previousMenus[mmgr->inMenuDepth] = mmgr->currentMenu;
    mmgr->currentMenu->switchTo = NULL;
    mmgr->currentMenu = newMenu;
    mmgr->inMenuDepth++;
}
