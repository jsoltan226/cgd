#include "parallax-bg.h"
#include "core/log.h"
#include "asset-loader/asset.h"
#include <SDL2/SDL_blendmode.h>
#include <SDL2/SDL_pixels.h>
#include <SDL2/SDL_render.h>
#include <stdio.h>

struct parallax_bg * parallax_bg_init(struct parallax_bg_config *cfg, SDL_Renderer *renderer)
{
    struct parallax_bg *bg = calloc(1, sizeof(struct parallax_bg));
    if (bg == NULL) {
        s_log_error("parallax-bg", "calloc() failed for struct parallax_bg!");
        return NULL;
    }

    /* Allocate space for all the background layers */
    bg->layer_count = cfg->layer_count;
    bg->layers = malloc(cfg->layer_count * sizeof(struct parallax_bg_layer));
    if (bg->layers == NULL) {
        s_log_error("parallax-bg", "malloc() failed for parallax_bg layers!");
        parallax_bg_destroy(bg);
        return NULL;
    }


    /* Initialize all the layers one by one */
    for(u32 i = 0; i < cfg->layer_count; i++) {
        struct parallax_bg_layer *item = &bg->layers[i];

        /* Load the texture and enable alpha blending */
        item->asset = asset_load(cfg->layer_img_filepaths[i], renderer);
        if (item->asset == NULL) {
            s_log_error("parallax-bg", "Failed to load asset \"%s\"!",
                cfg->layer_img_filepaths[i]
            );
            parallax_bg_destroy(bg);
            return NULL;
        }

        SDL_SetTextureBlendMode(item->asset->texture, SDL_BLENDMODE_BLEND);

        /* Set all other members to the appropriate values */
        SDL_QueryTexture(item->asset->texture, NULL, NULL, &item->w, &item->h);
        item->x = 0;
        item->speed = cfg->layer_speeds[i];
    }

    return bg;
}

void parallax_bg_update(struct parallax_bg *bg)
{
    if (bg == NULL || bg->layers == NULL) return;

    /* Simulate an infinite scrolling effect on all the layers */
    for(u32 i = 0; i < bg->layer_count; i++) {
        bg->layers[i].x += bg->layers[i].speed;
        if(bg->layers[i].x > bg->layers[i].w / 2 || bg->layers[i].x < -(bg->layers[i].w / 2))
            bg->layers[i].x = 0;
    }
}

void parallax_bg_draw(struct parallax_bg *bg, SDL_Renderer *renderer)
{
    if (bg == NULL || bg->layers == NULL) return;

    /* Draw all the layers */
    for(u32 i = 0; i < bg->layer_count; i++) {
        SDL_Rect rect = { bg->layers[i].x, 0, bg->layers[i].w / 2, bg->layers[i].h };
        SDL_RenderCopy(renderer, bg->layers[i].asset->texture, &rect, NULL);
    }
}

void parallax_bg_destroy(struct parallax_bg *bg)
{
    if (bg == NULL) return;

    /* Destroy the layers' textures, and then the whole bg->layers array */
    if (bg->layers != NULL) {
        for(u32 i = 0; i < bg->layer_count; i++)
            asset_destroy(bg->layers[i].asset);

        free(bg->layers);
        bg->layers = NULL;
    }

    /* Free the BG struct */
    free(bg);
    bg = NULL;
}
