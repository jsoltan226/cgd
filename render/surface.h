#ifndef R_SURFACE_H_
#define R_SURFACE_H_

#include "rctx.h"
#include <core/pixel.h>
#include <core/shapes.h>

/* A surface is just a 2d array (with a certain width and height)
 * of pixels (of a certain format).
 *
 * Despite the `r_` prefix, the surface can very much be used
 * outside of just the rendering subystem and its purposes.
 */

struct r_surface {
    struct pixel_flat_data data;    /* The pixel buffer */
    rect_t data_rect;               /* Always { 0, 0, data->w, data->h } */
    pixelfmt_t color_format;        /* The pixel format */
};

/* Create a new surface and allocate its pixel buffer to be of width `w`
 * and height `h`, and set the pixel format to `color_format`. */
struct r_surface * r_surface_create(u32 w, u32 h, pixelfmt_t color_format);

/* Create a new surface with the pre-allocate buffer `pixels`
 * and the pixel format `color_format`. Always succeeds. */
struct r_surface * r_surface_init(struct pixel_flat_data *pixels,
    pixelfmt_t color_format);

/* Blit (copy) the pixels in `src` limited by the area `src_rect`
 * onto the area `dst_rect` on `dst`'s pixels.
 * If needed, scaling and/or pixel format conversion is performed.
 * Only pixels that fit in the buffer's dimensions AND the area
 * are selected; the rest get cut off.
 *
 * Important note: While no errors should ever occur, the args are checked
 * with assertions (i.e. the program will crash if they are invalid!)
 *
 * Another Important note: The buffers in `src` and `dst` can NOT overlap!
 * If they do the behavior is undefined!
*/
void r_surface_blit(struct r_surface *dst, const struct r_surface *src,
    const rect_t *src_rect, const rect_t *dst_rect);

/* Performs a blit from `src` to the rendering buffer of `rctx` */
void r_surface_render(struct r_ctx *rctx, const struct r_surface *src,
    const rect_t *src_rect, const rect_t *dst_rect);

/* Destroy the surface at `**surface_p`,
 * and set `*surface_p` to `NULL`. */
void r_surface_destroy(struct r_surface **surface_p);

#endif /* R_SURFACE_H_ */
