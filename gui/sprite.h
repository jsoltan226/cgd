#ifndef SPRITE_H_
#define SPRITE_H_

#include <core/shapes.h>
#include <render/rctx.h>
#include <asset-loader/asset.h>

typedef struct {
    rect_t hitbox;
    rect_t src_rect, dst_rect;
    struct asset *asset;
} sprite_t;

struct sprite_config {
    u8 magic;
    rect_t hitbox, src_rect, dst_rect;
    filepath_t texture_filepath;
};

sprite_t * sprite_init(const struct sprite_config *cfg, struct r_ctx *rctx);
void sprite_draw(sprite_t *spr, struct r_ctx *rctx);
void sprite_destroy(sprite_t *spr);

#endif /* SPRITE_H_ */
