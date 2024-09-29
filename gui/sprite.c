#include "sprite.h"
#include "menu.h"
#include <asset-loader/asset.h>
#include <core/log.h>
#include <core/util.h>
#include <SDL2/SDL_render.h>

#define MODULE_NAME "sprite"

sprite_t * sprite_init(const struct sprite_config *cfg, SDL_Renderer *renderer)
{
    u_check_params(cfg != NULL && renderer != NULL)

    if (cfg->magic != MENU_CONFIG_MAGIC) {
        s_log_error("The config struct is invalid");
        return NULL;
    }

    sprite_t *spr = malloc(sizeof(sprite_t));
    s_assert(spr != NULL, "malloc() failed for new sprite");

    spr->src_rect = cfg->src_rect;
    spr->dst_rect = cfg->dst_rect;
    spr->hitbox = cfg->hitbox;

    spr->asset = asset_load(cfg->texture_filepath, renderer);
    if (spr->asset == NULL) {
        s_log_error("Failed to load asset \"%s\"!", cfg->texture_filepath);
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
