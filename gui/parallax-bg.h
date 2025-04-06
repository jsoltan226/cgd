#ifndef PARALLAX_BG_H_
#define PARALLAX_BG_H_

#include <core/int.h>
#include <core/vector.h>
#include <render/rctx.h>
#include <asset-loader/asset.h>

/* A single layer of a parallax background.
 * Should be an image that can "wrap" around
 * to give the illusion of infinite scrolling. */
struct parallax_bg_layer {
    struct asset *asset; /* The handle to the image */

    i32 w, h; /* The destination width and height of the image */

    i32 speed; /* The scrolling speed - how much `x` is decremented each tick */

    i32 x; /* The current position in the image */
};

/* A parallax backround - a backround with multiple layers, where the
 * background ones scroll by at a faster pace than the foreground ones,
 * giving a nice illusion of depth.
 *
 * The images should be made in such a way that the left end
 * "connects" to the right end, so that they can "wrap around"
 * and therefore scroll infinitely. */
struct parallax_bg {
    /* All the layers that are to be drawn,
     * sorted from backround to foreground */
    VECTOR(struct parallax_bg_layer) layers;

/* The maximum amount of layers in a background */
#define PARALLAX_BG_MAX_LAYERS  32
};

/* The information needed to create a parallax bg layer */
struct parallax_bg_layer_config {
    u8 magic; /* Must be `MENU_CONFIG_MAGIC`. */

    const char *filepath; /* The path to the image */

    i32 speed; /* The speed at which the image will be scrolling */
};

/* The information needed to create a parallax background */
struct parallax_bg_config {
    /* The configs of all the layers that are to be created */
    const struct parallax_bg_layer_config *layer_cfgs;
};

/* Creates a new parralax background based on the information in `cfg`.
 * Returns `NULL` on failure. */
struct parallax_bg * parallax_bg_init(const struct parallax_bg_config *cfg);

/* Updates all the layer positions in `bg`. */
void parallax_bg_update(struct parallax_bg *bg);

/* Draws all the layers of `bg` with the rendering context `rctx`. */
void parallax_bg_draw(struct parallax_bg *bg, struct r_ctx *rctx);

/* Destroys all the layers in `*bg_p`
 * and invalidates the handle by setting `*bg_p` to `NULL`. */
void parallax_bg_destroy(struct parallax_bg **bg_p);

#endif /* PARALLAX_BG_H_ */
