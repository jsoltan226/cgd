#include "parallax-bg.h"
#include <cgd/asset-loader/asset.h>
#include <SDL2/SDL_blendmode.h>
#include <SDL2/SDL_pixels.h>
#include <SDL2/SDL_render.h>
#include <stdio.h>
#include <assert.h>
#include <sys/types.h>

bg_ParallaxBG* bg_initBG(bg_BGConfig* cfg, SDL_Renderer* renderer)
{
    /* Allocate space for the BG struct */
    bg_ParallaxBG* bg = malloc(sizeof(bg_ParallaxBG));
    assert(bg != NULL);

    /* Allocate space for all the background layers */
    bg->layerCount = cfg->layerCount;
    bg->layers = malloc(cfg->layerCount * sizeof(bg_Layer));
    assert(bg->layers != NULL);


    /* Initialize all the layers one by one */
    for(int i = 0; i < cfg->layerCount; i++)
    {
        bg_Layer* item = &bg->layers[i];
        
        /* Load the texture and enable alpha blending */
        item->asset = asset_load(cfg->layerImageFilePaths[i], renderer);
        SDL_SetTextureBlendMode(item->asset->texture, SDL_BLENDMODE_BLEND);

        /* Set all other members to the appropriate values */
        SDL_QueryTexture(item->asset->texture, NULL, NULL, &item->w, &item->h);
        item->x = 0;
        item->speed = cfg->layerSpeeds[i];
    }

    return bg;
}

void bg_updateBG(bg_ParallaxBG* bg)
{
    /* Simulate an infinite scrolling effect on all the layers */ 
    for(int i = 0; i < bg->layerCount; i++)
    {
        bg->layers[i].x += bg->layers[i].speed;
        if(bg->layers[i].x > bg->layers[i].w / 2)
            bg->layers[i].x = 0;
    }
}

void bg_drawBG(bg_ParallaxBG* bg, SDL_Renderer* renderer)
{
    /* Draw all the layers */
    for(int i = 0; i < bg->layerCount; i++)
    {
        SDL_Rect rect = { bg->layers[i].x, 0, bg->layers[i].w / 2, bg->layers[i].h };
        SDL_RenderCopy(renderer, bg->layers[i].asset->texture, &rect, NULL);
    }
}

void bg_destroyBG(bg_ParallaxBG* bg)
{
    /* Destroy the layers' textures, and then the whole bg->layers array */
    for(int i = 0; i < bg->layerCount; i++)
        asset_destroy(bg->layers[i].asset);
        
    free(bg->layers);
    bg->layers = NULL;
    
    /* Free the BG struct */
    free(bg);
    bg = NULL;
}
