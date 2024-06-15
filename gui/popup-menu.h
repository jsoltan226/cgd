#ifndef POPUP_MENU_H
#define POPUP_MENU_H

#include "buttons.h"
#include "event-listener.h"
#include "sprite.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_render.h>

typedef struct {
    struct button *buttons;
    struct event_listener *eventListeners;
    sprite_t *sprites;
} pum_PopUpMenu;

typedef struct {
} pum_PopUpMenuConfig;

pum_PopUpMenu* pum_initPopUpMenu(pum_PopUpMenuConfig *cfg, SDL_Renderer *r);
void pum_updatePopUpMenu(pum_PopUpMenu *pum);
void pum_drawPopUpMenu(pum_PopUpMenu *pum, SDL_Renderer *r);
void pum_destroyPopUpMenu(pum_PopUpMenu *pum);

#endif
