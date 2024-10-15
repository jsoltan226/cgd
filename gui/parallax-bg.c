#include "parallax-bg.h"
#include "menu.h"
#include <core/log.h>
#include <core/util.h>
#include <core/vector.h>
#include <render/rctx.h>
#include <render/surface.h>
#include <asset-loader/asset.h>
#include <stdio.h>

#define MODULE_NAME "parallax-bg"

struct parallax_bg * parallax_bg_init(
    const struct parallax_bg_config *cfg,
    struct r_ctx *rctx)
{
    u_check_params(cfg != NULL && rctx != NULL);

    struct parallax_bg *bg = calloc(1, sizeof(struct parallax_bg));
    s_assert(bg != NULL, "calloc() failed for struct parallax_bg");

    /* Allocate space for all the background layers */
    bg->layers = vector_new(struct parallax_bg_layer);
    if (bg->layers == NULL) {
        s_log_error("Failed to initialize the layers vector");
        parallax_bg_destroy(bg);
        return NULL;
    }


    /* Initialize all the layers one by one */
    u32 i = 0;
    while (cfg->layer_cfgs[i].magic == MENU_CONFIG_MAGIC && i < PARALLAX_BG_MAX_LAYERS) {

        /* Load the texture and enable alpha blending */
        struct asset *asset = asset_load(cfg->layer_cfgs[i].filepath, rctx);
        if (asset == NULL) {
            s_log_error("Failed to load asset \"%s\"!",
                cfg->layer_cfgs[i].filepath
            );
            parallax_bg_destroy(bg);
            return NULL;
        }

        /* Get the texture parameters */
        vector_push_back(bg->layers, (struct parallax_bg_layer){
            .asset = asset,
            .w = asset->surface->data.w,
            .h = asset->surface->data.h,
            .x = 0,
            .speed = cfg->layer_cfgs[i].speed
        });
        i++;
    }

    s_log_debug("Initialized %u layers", i);

    return bg;
}

void parallax_bg_update(struct parallax_bg *bg)
{
    if (bg == NULL || bg->layers == NULL) return;

    /* Simulate an infinite scrolling effect on all the layers */
    for(u32 i = 0; i < vector_size(bg->layers); i++) {
        bg->layers[i].x += bg->layers[i].speed;
        if(bg->layers[i].x > bg->layers[i].w / 2 || bg->layers[i].x < -(bg->layers[i].w / 2))
            bg->layers[i].x = 0;
    }
}

void parallax_bg_draw(struct parallax_bg *bg, struct r_ctx *rctx)
{
    if (bg == NULL || bg->layers == NULL || rctx == NULL) return;

    /* Draw all the layers */
    for(u32 i = 0; i < vector_size(bg->layers); i++) {
        const rect_t dst_rect = {
            bg->layers[i].x,
            0,
            bg->layers[i].w / 2,
            bg->layers[i].h
        };
        r_surface_blit(bg->layers[i].asset->surface, NULL, &dst_rect);
    }
}

void parallax_bg_destroy(struct parallax_bg *bg)
{
    if (bg == NULL) return;

    if (bg->layers != NULL) {
        for(u32 i = 0; i < vector_size(bg->layers); i++)
            asset_destroy(bg->layers[i].asset);

        vector_destroy(bg->layers);
    }

    free(bg);
}
