#ifndef PARALLAXBG_H
#define PARALLAXBG_H

#include <SDL2/SDL.h>

typedef struct {
    SDL_Texture* texture;
    int w, h;
    unsigned int speed;
    unsigned int x;
} bg_Layer;

typedef struct {
    bg_Layer* layers;
    int layerCount;
} bg_ParallaxBG;

typedef struct {
    const char** layerImageFilePaths;
    int* layerSpeeds;
    int layerCount;
} bg_BGConfig;

bg_ParallaxBG* bg_initBG(bg_BGConfig* cfg, SDL_Renderer* r);
extern void bg_updateBG(bg_ParallaxBG* bg);
extern void bg_drawBG(bg_ParallaxBG* bg, SDL_Renderer* renderer);
extern void bg_destroyBG(bg_ParallaxBG* bg);

#endif
