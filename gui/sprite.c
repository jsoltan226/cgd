#include "sprite.h"
#include "asset-loader/asset.h"
#include "core/log.h"
#include <SDL2/SDL_render.h>

sprite_t * sprite_init(struct sprite_config *cfg, SDL_Renderer *renderer)
{
    /* Allocate the sprite struct on the heap */
    sprite_t *spr = malloc(sizeof(sprite_t));
    if (spr == NULL) {
        s_log_error("sprite", "malloc() failed for new sprite");
        return NULL;
    }

    spr->src_rect = cfg->src_rect;
    spr->dst_rect = cfg->dst_rect;
    spr->hitbox = cfg->hitbox;

    spr->asset = asset_load(cfg->texture_filepath, renderer);
    if (spr->asset == NULL) {
        s_log_error("sprite", "Failed to load asset \"%s\"!", cfg->texture_filepath);
        sprite_destroy(spr);
        return NULL;
    }

    return spr;
}

void sprite_draw(sprite_t *s, SDL_Renderer *renderer)
{
    if (s == NULL || renderer == NULL) return;

    SDL_RenderCopy(renderer,
        s->asset->texture,
        (const SDL_Rect *)&s->src_rect,
        (const SDL_Rect *)&s->dst_rect
    );
}

void sprite_destroy(sprite_t *spr)
{
    if (spr == NULL) return;

    asset_destroy(spr->asset);
    free(spr);
}
