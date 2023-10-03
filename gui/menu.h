#ifndef MENU_H
#define MENU_H

#include "buttons.h"
#include "sprite.h"
#include "parallax-bg.h"
#include "../user-input/mouse.h"
#include <SDL2/SDL_render.h>

typedef struct mn_Menu mn_Menu;
struct mn_Menu {
    spr_Sprite** sprites;
    int spriteCount;
    btn_Button** buttons;
    int buttonCount;
    bg_ParallaxBG* bg;
    mn_Menu* switchTo;      
};

typedef struct {
    bg_BGConfig bgConfig;
    spr_SpriteConfig* spriteCfgs;
    int spriteCount;
    spr_SpriteConfig* buttonCfgs;
    int buttonCount;
} mn_MenuConfig;

mn_Menu* mn_initMenu(mn_MenuConfig* cfg, SDL_Renderer* r);
void mn_updateMenu(mn_Menu* menu, input_Mouse mouse);
void mn_drawMenu(mn_Menu* menu, SDL_Renderer* r, bool displayButtonHitboxes);
void mn_destroyMenu(mn_Menu* menu);

#endif
