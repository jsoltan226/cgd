#include "sprite.h"
#include "menu.h"
#include "render/surface.h"
#include <asset-loader/asset.h>
#include <core/log.h>
#include <core/util.h>
#include <render/rctx.h>

#define MODULE_NAME "sprite"

sprite_t * sprite_init(const struct sprite_config *cfg, struct r_ctx *rctx)
{
    u_check_params(cfg != NULL && rctx != NULL)

    if (cfg->magic != MENU_CONFIG_MAGIC) {
        s_log_error("The config struct is invalid");
        return NULL;
    }

    sprite_t *spr = malloc(sizeof(sprite_t));
    s_assert(spr != NULL, "malloc() failed for new sprite");

    spr->src_rect = cfg->src_rect;
    spr->dst_rect = cfg->dst_rect;
    spr->hitbox = cfg->hitbox;

    spr->asset = asset_load(cfg->texture_filepath, rctx);
    if (spr->asset == NULL) {
        s_log_error("Failed to load asset \"%s\"!", cfg->texture_filepath);
        sprite_destroy(spr);
        return NULL;
    }

    return spr;
}

void sprite_draw(sprite_t *s, struct r_ctx *rctx)
{
    if (s == NULL || rctx) return;

    r_surface_blit(s->asset->surface,
        &s->src_rect,
        &s->dst_rect
    );
}

void sprite_destroy(sprite_t *spr)
{
    if (spr == NULL) return;

    asset_destroy(spr->asset);
    spr->asset = NULL;
    free(spr);
}
