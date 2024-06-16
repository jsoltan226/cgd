#ifndef MENUMGR_H
#define MENUMGR_H

#include "event-listener.h"
#include "menu.h"
#include <SDL2/SDL_render.h>
#include <stdbool.h>

typedef struct {
    struct Menu *currentMenu;
    struct Menu **previousMenus;
    int inMenuDepth;
    struct Menu **fullMenuList;
    int menuCount;
    struct event_listener **globalEventListeners;
    int globalEventListenerCount;
} mmgr_MenuManager;

typedef struct {
    int menuCount;
    struct menu_config *menus;
    struct menu_onevent_config *globalEventListenerOnEventCfgs;
    struct event_listener_config *globalEventListenerCfgs;
    int globalEventListenerCount;
} mmgr_MenuManagerConfig;

mmgr_MenuManager *mmgr_initMenuManager(mmgr_MenuManagerConfig* cfg, SDL_Renderer* renderer,
    struct keyboard *keyboard, struct mouse *mouse);

void mmgr_updateMenuManager(mmgr_MenuManager *mmgr,
    struct keyboard *keyboard, struct mouse *mouse,
    bool paused);

void mmgr_drawMenuManager(mmgr_MenuManager *mmgr, SDL_Renderer *r);
void mmgr_destroyMenuManager(mmgr_MenuManager *mmgr);
void mmgr_switchMenu(mmgr_MenuManager *mmgr, u64 switchTo);
void mmgr_goBackMenu(mmgr_MenuManager *mmgr);

#endif
