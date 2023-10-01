#ifndef MENUMGR_H
#define MENUMGR_H

#include "menu.h"
#include <SDL2/SDL_render.h>

typedef struct {
    mn_Menu* currentMenu;
    mn_Menu** previousMenus;
    int inMenuDepth;
    mn_Menu** fullMenuList;
    int menuCount;
} mmgr_MenuManager;

void mmgr_initMenuManager(mmgr_MenuManager* mmgr, mn_Menu** fullMenuList, int menuCount, mn_Menu* firstMenu);
void mmgr_updateMenuManager(mmgr_MenuManager* mmgr, input_Keyboard keyboard, input_Mouse mouse);
void mmgr_drawMenuManager(mmgr_MenuManager* mmgr, SDL_Renderer* r, bool displayButtonHitboxes);
void mmgr_destroyMenuManager(mmgr_MenuManager* mmgr);
void mmgr_switchMenu(mmgr_MenuManager* mmgr, mn_Menu* newMenu);
void mmgr_goBackMenu(mmgr_MenuManager* mmgr);

#endif