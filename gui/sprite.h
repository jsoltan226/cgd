#ifndef SPRITE_H_
#define SPRITE_H_

#include <core/util.h>
#include <core/shapes.h>
#include <render/rctx.h>
#include <asset-loader/asset.h>

/* Sprite - an image that can be drawn to the screen, that has a hitbox */
struct sprite {
    rect_t hitbox; /* The hitbox */

    /* The part of the image that is to be drawn.
     * `x` and `y` are an offset from the top left corner of the image,
     * while `w` and `h` define the area of the rect.
     *
     * Any part of the rectangle that is outside the image
     * will be "chopped off" and ignored. */
    rect_t src_rect;

    /* The position and dimensions of where the image should be drawn.
     * `x` and `y` are an offset from the top left corner of the window,
     * while `w` and `h` define the area of the rect.
     *
     * Any part of the rectangle that is outside the window
     * will be "chopped off" and not ignored. */
    rect_t dst_rect;

    /* The handle to the image to be drawn */
    struct asset *asset;
};

/* This struct holds the information needed to create a sprite. */
struct sprite_config {
    rect_t hitbox; /* The hitbox of the sprite */

    /* The source and destination positions and dimensions */
    rect_t src_rect, dst_rect;

    /* The path to the sprite's image */
    const char *texture_filepath;
};

/* Creates a new sprite with the information in `cfg`.
 * Returns `NULL` on failure. */
struct sprite * sprite_init(const struct sprite_config *cfg);

/* Draws the sprite `spr` with the renderer `rctx`. */
void sprite_draw(struct sprite *spr, struct r_ctx *rctx);

/* Destroys the sprite pointed to by `spr_p`,
 * and invalidates the handle by setting `*spr_p` to `NULL`. */
void sprite_destroy(struct sprite **spr_p);

#endif /* SPRITE_H_ */
