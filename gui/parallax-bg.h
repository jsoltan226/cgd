#pragma once
#ifndef PARALLAXBG_H
#define PARALLAXBG_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

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

extern void bg_initBG(bg_ParallaxBG* bg, SDL_Renderer* renderer, const char** layerImageFilePaths, int* layerSpeeds, int layerCount);
extern void bg_updateBG(bg_ParallaxBG* bg);
extern void bg_drawBG(bg_ParallaxBG* bg, SDL_Renderer* renderer);
extern void bg_destroyBG(bg_ParallaxBG* bg);

#endif
