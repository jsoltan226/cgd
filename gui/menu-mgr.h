#ifndef MENUMGR_H
#define MENUMGR_H

#include "menu.h"

typedef struct {
    mn_Menu* currentMenu;
    mn_Menu** previousMenus;
    int inMenuDepth;
} mmgr_MenuManager;

void mmgr_initMenuManager();
void mmgr_updateMenuManager();
void mmgr_drawMenuManager();
void mmgr_destroyMenuManager(mmgr_MenuManager* mmgr);

#endif
