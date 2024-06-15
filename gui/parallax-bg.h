#ifndef PARALLAX_BG_H_
#define PARALLAX_BG_H_

#include <SDL2/SDL.h>
#include <SDL2/SDL_render.h>
#include "core/int.h"
#include "asset-loader/asset.h"

struct parallax_bg_layer {
    struct asset *asset;
    i32 w, h;
    i32 speed;
    i32 x;
};

struct parallax_bg {
    struct parallax_bg_layer *layers;
    u32 layer_count;
};

struct parallax_bg_config {
    const char **layer_img_filepaths;
    i32 *layer_speeds;
    u32 layer_count;
};

struct parallax_bg * parallax_bg_init(struct parallax_bg_config *cfg, SDL_Renderer *renderer);
void parallax_bg_update(struct parallax_bg *bg);
void parallax_bg_draw(struct parallax_bg *bg, SDL_Renderer *renderer);
void parallax_bg_destroy(struct parallax_bg *bg);

#endif /* PARALLAX_BG_H_ */
