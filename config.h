#include "main-menu/buttons.h"
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

static const SDL_Color rendererBg = { 30, 30, 30, 100 };
static const SDL_Rect gameRect = { 20, 20, WINDOW_WIDTH - 40, WINDOW_HEIGHT - 40 };
static const int mainBGLayerCount = 5;

static const char* mainBGLayerImageFilePaths[] = {
    "assets/main-menu/background/layer_01.png",
    "assets/main-menu/background/layer_02.png",
    "assets/main-menu/background/layer_03.png", 
    "assets/main-menu/background/layer_04.png",
    "assets/main-menu/background/layer_05.png",
};
static int mainBGLayerSpeeds[] = {
    1, 2, 4, 16, 16
};

static int testButtonOnClick(){
    printf("Button Pressed!\n");
    return EXIT_SUCCESS;
};

/* tb is test button */
#define tb_SrcWidth 780.f
#define tb_SrcHeight 280.f
#define tb_HitboxSrcX 0
#define tb_HitboxSrcY 121.f
#define tb_HitboxSrcWidth 770.f
#define tb_HitboxSrcHeight 160.f

static const SDL_Rect testButtonHitbox = {
    TESTBUTTON_X + (int)( TESTBUTTON_WIDTH  * ( tb_HitboxSrcX/tb_SrcWidth  )),
    TESTBUTTON_Y + (int)( TESTBUTTON_HEIGHT * ( tb_HitboxSrcY/tb_SrcHeight )),

    (int)( TESTBUTTON_WIDTH  * ( tb_HitboxSrcWidth/tb_SrcWidth   )),
    (int)( TESTBUTTON_HEIGHT * ( tb_HitboxSrcHeight/tb_SrcHeight )) 
},
             testButtonSrcRect = { 0, 0, 780, 280 },
             testButtonDestRect = { 20, 20, 200, 200 };
            
static const char* testButtonFilePath = "assets/main-menu/buttons/testButton.png";
static bool displayButtonHitboxOutlines = false;
