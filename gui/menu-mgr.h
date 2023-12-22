#ifndef MENUMGR_H
#define MENUMGR_H

#include <cgd/gui/event-listener.h>
#include <cgd/gui/menu.h>
#include <SDL2/SDL_render.h>
#include <stdbool.h>

typedef struct {
    mn_Menu *currentMenu;
    mn_Menu **previousMenus;
    int inMenuDepth;
    mn_Menu **fullMenuList;
    int menuCount;
    evl_EventListener **globalEventListeners;
    int globalEventListenerCount;
} mmgr_MenuManager;

typedef struct {
    int menuCount;
    mn_MenuConfig *menus;
    mn_OnEventConfig *globalEventListenerOnEventCfgs;
    evl_EventListenerConfig *globalEventListenerCfgs;
    int globalEventListenerCount;
} mmgr_MenuManagerConfig;

mmgr_MenuManager *mmgr_initMenuManager(mmgr_MenuManagerConfig* cfg, SDL_Renderer* renderer, kb_Keyboard *keyboard, ms_Mouse *mouse);
void mmgr_updateMenuManager(mmgr_MenuManager *mmgr, kb_Keyboard *keyboard, ms_Mouse *mouse, bool paused);
void mmgr_drawMenuManager(mmgr_MenuManager *mmgr, SDL_Renderer *r, bool displayButtonHitboxes);
void mmgr_destroyMenuManager(mmgr_MenuManager *mmgr);
void mmgr_switchMenu(mmgr_MenuManager *mmgr, mn_ID switchTo);
void mmgr_goBackMenu(mmgr_MenuManager *mmgr);

#endif
