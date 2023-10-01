#include "menu-mgr.h"
#include "menu.h"
#include <SDL2/SDL_render.h>
#include <stdlib.h>
#include <assert.h>
#include <error.h>

void mmgr_initMenuManager(mmgr_MenuManager* mmgr, mn_Menu** fullMenuList, int menuCount, mn_Menu* firstMenu)
{
    assert(menuCount >= 0);
    if((fullMenuList == NULL || menuCount <= 0) && firstMenu != NULL){
        mmgr->menuCount = 0;
        mmgr->fullMenuList = NULL;
        mmgr->inMenuDepth = -1;
        mmgr->previousMenus = NULL;
        mmgr_switchMenu(mmgr, firstMenu);
    } else if(!(fullMenuList == NULL || menuCount <= 0) && firstMenu == NULL){
        mmgr->menuCount = menuCount;
        mmgr->fullMenuList = calloc(menuCount, sizeof(mn_Menu*));
        mmgr->currentMenu = fullMenuList[0];
    } else if(!(fullMenuList == NULL || menuCount <= 0) && firstMenu != NULL){
        mmgr->menuCount = menuCount;
        mmgr->fullMenuList = fullMenuList;
        mmgr->currentMenu = firstMenu;
    } else {
        mmgr->fullMenuList = NULL;
        mmgr->menuCount = 0;
        mmgr->currentMenu = NULL;
    }

    mmgr->inMenuDepth = 0;
    mmgr->previousMenus = NULL;
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
}

void mmgr_goBackMenu(mmgr_MenuManager* mmgr)
{
    if(mmgr->inMenuDepth <= 0)
        return;
    else {
        mmgr->currentMenu = mmgr->previousMenus[mmgr->inMenuDepth - 1];
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
    mmgr->currentMenu = newMenu;
    mmgr->inMenuDepth++;
}
