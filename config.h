#include <SDL2/SDL.h>
#include <SDL2/SDL_pixels.h>
#include <SDL2/SDL_rect.h>

#define WINDOW_TITLE                    "cgd"
#define WINDOW_X                        SDL_WINDOWPOS_CENTERED
#define WINDOW_Y                        SDL_WINDOWPOS_CENTERED
#define WINDOW_WIDTH                    750
#define WINDOW_HEIGHT                   500
#define WINDOW_FLAGS                    SDL_WINDOW_OPENGL

#define RENDERER_FLAGS                  SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_TARGETTEXTURE
#define FRAME_DURATION                  16


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
