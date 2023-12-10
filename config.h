#ifndef CONFIG_H
#define CONFIG_H

#include "gui/event-listener.h"
#include "gui/menu.h"
#include "gui/menu-mgr.h"
#include "gui/on-event.h"
#include "gui/parallax-bg.h"
#include "gui/sprite.h"
#include "gui/buttons.h"
#include "user-input/keyboard.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_pixels.h>
#include <SDL2/SDL_rect.h>
#include <stdlib.h>
#include <stdbool.h>

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

static bool running = true;
static bool displayButtonHitboxOutlines = false;
static bool paused = false;

const SDL_Color rendererBg = { 30, 30, 30, 100 };
const SDL_Rect gameRect = { 20, 20, WINDOW_WIDTH - 40, WINDOW_HEIGHT - 40 };

/* tb is test button */
#define tb_SrcWidth 780.f
#define tb_SrcHeight 280.f
#define tb_HitboxSrcX 0
#define tb_HitboxSrcY 121.f
#define tb_HitboxSrcWidth 770.f
#define tb_HitboxSrcHeight 160.f

enum MenuIDs {
    MAIN_MENU,
    SUB_MENU
};
enum MainMenuButtonIDs {
    MAIN_MENU_BUTTON_TESTBUTTON
};

const mmgr_MenuManagerConfig menuManagerConfig = {
    .menuCount = 2,
    .menus = (mn_MenuConfig*)(mn_MenuConfig[]){
        {
            .bgConfig = {
                .layerImageFilePaths = (const char**)(const char*[]){
                    "/gui/background/layer_01.png",
                    "/gui/background/layer_02.png",
                    "/gui/background/layer_03.png",
                    "/gui/background/layer_04.png",
                    "/gui/background/layer_05.png",
                },
                .layerSpeeds = (int*)(int[]){ 1, 2, 4, 16, 16 },
                .layerCount = 5,
            },

            .spriteInfo = { 0 },

            .buttonInfo = {
                .count = 1,
                .cfgs = (mn_ButtonConfig*)(mn_ButtonConfig[]){
                    {
                        .spriteCfg = {
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

                            .textureFilePath = "/gui/buttons/testButton.png"
                        },
                        .onClickCfg = {
                            .onEventType = MN_ONEVENT_SWITCHMENU,
                            .onEventArgs = {
                                .switchDestMenuID = SUB_MENU,
                            }, 
                        },
                    },
                },
            },
            .eventListenerInfo = { 0 },

            .id = MAIN_MENU,

        },
        {
            .bgConfig = { 0 },

            .spriteInfo = { 0 },

            .buttonInfo = {
                .count = 1,
                .cfgs = (mn_ButtonConfig*)(mn_ButtonConfig[]){
                    {
                        .spriteCfg = {
                            /* HITBOX */
                            .hitbox = {
                                .x = 100 + TESTBUTTON_X + (int)( TESTBUTTON_WIDTH  * ( tb_HitboxSrcX/tb_SrcWidth  )),
                                .y = 150 + TESTBUTTON_Y + (int)( TESTBUTTON_HEIGHT * ( tb_HitboxSrcY/tb_SrcHeight )),

                                .w = (int)( TESTBUTTON_WIDTH  * ( tb_HitboxSrcWidth/tb_SrcWidth   )),
                                .h = (int)( TESTBUTTON_HEIGHT * ( tb_HitboxSrcHeight/tb_SrcHeight )) 
                            },

                            /* SOURCE RECT */
                            .srcRect = { 0, 0, 780, 280 },

                            /* DESTINATION RECT */
                            .destRect = { 100 + 20, 150 + 20, 200, 200 },

                            /* SDL_TEXTURE POINTER, INITIALIZED AT RUNTIME */
                            .textureFilePath = "/gui/buttons/testButton.png"
                        },
                        .onClickCfg = {
                            .onEventType = MN_ONEVENT_SWITCHMENU,
                            .onEventArgs = { .switchDestMenuID = MAIN_MENU },
                        },
                    }
                },
            },

            .eventListenerInfo = {
                .count = 1,
                .cfgs = (mn_eventListenerConfig*)(mn_eventListenerConfig[]){
                    {
                        .eventListenerCfg = {
                            .type = EVL_EVENT_KEYBOARD_KEYDOWN,
                            .targetInfo = { .keycode = KB_KEYCODE_P },
                        },
                        .onEventCfg = {
                            .onEventType = MN_ONEVENT_PRINTMESSAGE,
                            .onEventArgs = { .message = "hello!\n" },
                        },
                    }
                },
            },

            .id = SUB_MENU,
        },
    },
    .globalEventListenerOnEventCfgs = (mn_OnEventConfig*)(mn_OnEventConfig[]){
        {
            .onEventType = MN_ONEVENT_QUIT,
            .onEventArgs = { .boolVarPtr = &running },
        },
        {
            .onEventType = MN_ONEVENT_PAUSE,
            .onEventArgs = { .boolVarPtr = &paused },
        },
        {
            .onEventType = MN_ONEVENT_FLIPBOOL,
            .onEventArgs = { .boolVarPtr = &displayButtonHitboxOutlines },
        }
    },
    .globalEventListenerCfgs = (evl_EventListenerConfig*)(evl_EventListenerConfig[]){
        {
            .type = EVL_EVENT_KEYBOARD_KEYUP,
            .targetInfo = { .keycode = KB_KEYCODE_Q },
        },
        {
            .type = EVL_EVENT_KEYBOARD_KEYUP,
            .targetInfo = { .keycode = KB_KEYCODE_ESCAPE },
        },
        {
            .type = EVL_EVENT_KEYBOARD_KEYDOWN,
            .targetInfo = { .keycode = KB_KEYCODE_H },
        },
    },
    .globalEventListenerCount = 3,
};

#endif
