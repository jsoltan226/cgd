#ifndef SPRITE_H_
#define SPRITE_H_

#include <SDL2/SDL.h>
#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_render.h>
#include "core/shapes.h"
#include "asset-loader/asset.h"

typedef struct {
    rect_t hitbox;
    rect_t src_rect, dst_rect;
    struct asset *asset;
} sprite_t;

struct sprite_config {
    rect_t hitbox, src_rect, dst_rect;
    const char *texture_filepath;
};

sprite_t * sprite_init(struct sprite_config *cfg, SDL_Renderer *r);
void sprite_draw(sprite_t *spr, SDL_Renderer *r);
void sprite_destroy(sprite_t *spr);

#endif /* SPRITE_H_ */
