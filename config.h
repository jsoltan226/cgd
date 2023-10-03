#ifndef CONFIG_H
#define CONFIG_H

#include "gui/menu.h"
#include "gui/menu-mgr.h"
#include "gui/parallax-bg.h"
#include "gui/sprite.h"
#include "gui/buttons.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_pixels.h>
#include <SDL2/SDL_rect.h>
#include <stdlib.h>

#define WINDOW_TITLE                    "cgd"
#define WINDOW_X                        SDL_WINDOWPOS_CENTERED
#define WINDOW_Y                        SDL_WINDOWPOS_CENTERED
#define WINDOW_WIDTH                    750
#define WINDOW_HEIGHT                   500
#define WINDOW_FLAGS                    SDL_WINDOW_OPENGL

#define RENDERER_FLAGS                  SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_TARGETTEXTURE
#define FRAME_DURATION                  16

#define TESTBUTTON_X                    20
#define TESTBUTTON_Y                    20
#define TESTBUTTON_WIDTH                200
#define TESTBUTTON_HEIGHT               200

const SDL_Color rendererBg = { 30, 30, 30, 100 };
const SDL_Rect gameRect = { 20, 20, WINDOW_WIDTH - 40, WINDOW_HEIGHT - 40 };
const int mainBGLayerCount = 5;

const char* const mainBGLayerImageFilePaths[] = {
    "assets/gui/background/layer_01.png",
    "assets/gui/background/layer_02.png",
    "assets/gui/background/layer_03.png", 
    "assets/gui/background/layer_04.png",
    "assets/gui/background/layer_05.png",
};
const int mainBGLayerSpeeds[] = {
    1, 2, 4, 16, 16 
};

const bg_BGConfig mainMenuBgConfig = {
    .layerImageFilePaths = (const char**)mainBGLayerImageFilePaths,
    .layerSpeeds = (int*)mainBGLayerSpeeds,
    .layerCount = 5,
};
/* tb is test button */
#define tb_SrcWidth 780.f
#define tb_SrcHeight 280.f
#define tb_HitboxSrcX 0
#define tb_HitboxSrcY 121.f
#define tb_HitboxSrcWidth 770.f
#define tb_HitboxSrcHeight 160.f

static bool displayButtonHitboxOutlines = false;
const spr_SpriteConfig testButtonSpriteConfig = {
    /* HITBOX */
    .hitbox = {
        .x = TESTBUTTON_X + (int)( TESTBUTTON_WIDTH  * ( tb_HitboxSrcX/tb_SrcWidth  )),
        .y = TESTBUTTON_Y + (int)( TESTBUTTON_HEIGHT * ( tb_HitboxSrcY/tb_SrcHeight )),

        .w = (int)( TESTBUTTON_WIDTH  * ( tb_HitboxSrcWidth/tb_SrcWidth   )),
        .h = (int)( TESTBUTTON_HEIGHT * ( tb_HitboxSrcHeight/tb_SrcHeight )) 
    },

    /* SOURCE RECT */
    .srcRect = { 0, 0, 780, 280 },

    /* DESTINATION RECT */
    .destRect = { 20, 20, 200, 200 },

    /* SDL_TEXTURE POINTER, INITIALIZED DURING RUNTIME */
    .textureFilePath = "assets/gui/buttons/testButton.png"
};

enum MainMenuButtonIDs {
    MAIN_MENU_BUTTON_TESTBUTTON
};

const spr_SpriteConfig mainMenuButtonConfigs[] = {
    testButtonSpriteConfig
};

const mn_MenuConfig mainMenuConfig = {
    .bgConfig = mainMenuBgConfig,
    .spriteCfgs = NULL,
    .spriteCount = 0,
    .buttonCfgs = (spr_SpriteConfig*)mainMenuButtonConfigs,
    .buttonCount = 1,
};

const mn_MenuConfig menuManagerConfigs[] = {
    mainMenuConfig,
    NULL
};

const mmgr_MenuManagerConfig menuManagerConfig = {
    .menuCount = 2,
    .menus = (mn_MenuConfig*)menuManagerConfigs
};

#endif
