#ifndef PARALLAXBG_H
#define PARALLAXBG_H

#include <SDL2/SDL.h>
#include <cgd/util/int.h>
#include <cgd/asset-loader/asset.h>

typedef struct {
    struct asset *asset;
    i32 w, h;
    u32 speed;
    u32 x;
} bg_Layer;

typedef struct {
    bg_Layer* layers;
    i32 layerCount;
} bg_ParallaxBG;

typedef struct {
    const char** layerImageFilePaths;
    i32* layerSpeeds;
    i32 layerCount;
} bg_BGConfig;

bg_ParallaxBG* bg_initBG(bg_BGConfig* cfg, SDL_Renderer* r);
extern void bg_updateBG(bg_ParallaxBG* bg);
extern void bg_drawBG(bg_ParallaxBG* bg, SDL_Renderer* renderer);
extern void bg_destroyBG(bg_ParallaxBG* bg);

#endif
