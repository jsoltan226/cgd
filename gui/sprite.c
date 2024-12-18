#include "sprite.h"
#include "menu.h"
#include <asset-loader/asset.h>
#include <core/log.h>
#include <core/util.h>
#include <render/surface.h>

#define MODULE_NAME "sprite"

struct sprite * sprite_init(const struct sprite_config *cfg)
{
    u_check_params(cfg != NULL);

    if (cfg->magic != MENU_CONFIG_MAGIC) {
        s_log_error("The config struct is invalid");
        return NULL;
    }

    struct sprite *spr = malloc(sizeof(struct sprite));
    s_assert(spr != NULL, "malloc() failed for new sprite");

    spr->src_rect = cfg->src_rect;
    spr->dst_rect = cfg->dst_rect;
    spr->hitbox = cfg->hitbox;

    spr->asset = asset_load(cfg->texture_filepath);
    if (spr->asset == NULL) {
        s_log_error("Failed to load asset \"%s\"!", cfg->texture_filepath);
        sprite_destroy(&spr);
        return NULL;
    }

    return spr;
}

void sprite_draw(struct sprite *s, struct r_ctx *rctx)
{
    if (s == NULL || rctx == NULL) return;

    r_surface_render(rctx, s->asset->surface, &s->src_rect, &s->dst_rect);
}

void sprite_destroy(struct sprite **spr_p)
{
    if (spr_p == NULL || *spr_p == NULL) return;
    struct sprite *spr = *spr_p;

    asset_destroy(&spr->asset);
    u_nzfree(spr_p);
}
