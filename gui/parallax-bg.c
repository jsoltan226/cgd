#include "parallax-bg.h"
#include "../util.h"
#include <SDL2/SDL_blendmode.h>
#include <SDL2/SDL_pixels.h>
#include <SDL2/SDL_render.h>
#include <stdio.h>

void bg_initBG(bg_ParallaxBG* bg, SDL_Renderer* renderer, const char** restrict layerImageFilePaths, int* layerSpeeds, int layerCount)
{
    bg->layerCount = layerCount;
    bg->layers = (bg_Layer*)malloc(layerCount * sizeof(bg_Layer));
    for(int i = 0; i < layerCount; i++)
    {
        bg_Layer* item = &bg->layers[i];
        
        item->texture = u_loadImage(renderer, layerImageFilePaths[i]);
        SDL_SetTextureBlendMode(item->texture, SDL_BLENDMODE_BLEND);
        SDL_QueryTexture(item->texture, NULL, NULL, &item->w, &item->h);
        item->x = 0;
        item->speed = layerSpeeds[i];

    }
}

void bg_updateBG(bg_ParallaxBG* bg)
{
    for(int i = 0; i < bg->layerCount; i++)
    {
        bg->layers[i].x += bg->layers[i].speed;
        if(bg->layers[i].x > bg->layers[i].w / 2)
            bg->layers[i].x = 0;
    }
}

void bg_drawBG(bg_ParallaxBG* bg, SDL_Renderer* renderer)
{
    for(int i = 0; i < bg->layerCount; i++)
    {
        SDL_Rect rect = { bg->layers[i].x, 0, bg->layers[i].w / 2, bg->layers[i].h };
        SDL_RenderCopy(renderer, bg->layers[i].texture, &rect, NULL);
    }
}

void bg_destroyBG(bg_ParallaxBG *bg)
{
    for(int i = 0; i < bg->layerCount; i++)
        SDL_DestroyTexture(bg->layers[i].texture);
        
    free(bg->layers);
}
