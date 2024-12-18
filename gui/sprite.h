#ifndef SPRITE_H_
#define SPRITE_H_

#include <core/shapes.h>
#include <render/rctx.h>
#include <asset-loader/asset.h>

struct sprite {
    rect_t hitbox;
    rect_t src_rect, dst_rect;
    struct asset *asset;
};

struct sprite_config {
    u8 magic;
    rect_t hitbox, src_rect, dst_rect;
    filepath_t texture_filepath;
};

struct sprite * sprite_init(const struct sprite_config *cfg);
void sprite_draw(struct sprite *spr, struct r_ctx *rctx);
void sprite_destroy(struct sprite **spr_p);

#endif /* SPRITE_H_ */
