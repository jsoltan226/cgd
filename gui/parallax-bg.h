#ifndef PARALLAX_BG_H_
#define PARALLAX_BG_H_

#include <core/int.h>
#include <core/vector.h>
#include <render/rctx.h>
#include <asset-loader/asset.h>

struct parallax_bg_layer {
    struct asset *asset;
    i32 w, h;
    i32 speed;
    i32 x;
};

struct parallax_bg {
    VECTOR(struct parallax_bg_layer) layers;
};

#define PARALLAX_BG_MAX_LAYERS  32
struct parallax_bg_layer_config {
    u8 magic;
    const char *filepath;
    i32 speed;
};

struct parallax_bg_config {
    u8 magic;
    struct parallax_bg_layer_config layer_cfgs[PARALLAX_BG_MAX_LAYERS];
};

struct parallax_bg * parallax_bg_init(const struct parallax_bg_config *cfg);
void parallax_bg_update(struct parallax_bg *bg);
void parallax_bg_draw(struct parallax_bg *bg, struct r_ctx *rctx);
void parallax_bg_destroy(struct parallax_bg **bg_p);

#endif /* PARALLAX_BG_H_ */
