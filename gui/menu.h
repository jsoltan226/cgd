#ifndef MENU_H
#define MENU_H

#include "buttons.h"
#include "sprite.h"
#include "parallax-bg.h"
#include "../user-input/mouse.h"
#include <SDL2/SDL_render.h>

typedef struct mn_Menu mn_Menu;
struct mn_Menu {
    spr_Sprite* sprites;
    int spriteCount;
    btn_Button* buttons;
    int buttonCount;
    bg_ParallaxBG bg;
    mn_Menu* switchTo;      
};

typedef struct {
    struct {
        int layerCount;
        int* layerSpeeds;
        const char** layerFilePaths;
    } bgInfo;
    struct {
        int count;
        spr_Sprite* protos;
        const char** textureFilePaths;
    } spriteInfo;
    struct {
        int count;
        spr_Sprite* spriteProtos;
        const char** spriteTextureFilePaths;
    } buttonInfo;
} mn_PrototypeInfo;

void mn_initMenu(mn_Menu* m, mn_PrototypeInfo* protoInfo, SDL_Renderer* renderer);
void mn_updateMenu(mn_Menu* menu, input_Mouse mouse);
void mn_drawMenu(mn_Menu* menu, SDL_Renderer* r, bool displayButtonHitboxes);
void mn_destroyMenu(mn_Menu* menu);

#endif
