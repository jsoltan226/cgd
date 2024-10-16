#ifndef R_SURFACE_H_
#define R_SURFACE_H_

#include "rctx.h"
#include <core/pixel.h>
#include <platform/window.h>

struct r_surface {
    struct pixel_flat_data data;
    struct r_ctx *rctx;
    enum p_window_color_type color_format;
};

struct r_surface * r_surface_create(struct r_ctx *rctx, u32 w, u32 h,
    enum p_window_color_type color_format);

struct r_surface * r_surface_init(struct r_ctx *rctx,
    struct pixel_flat_data *pixels, enum p_window_color_type color_format);

void r_surface_blit(
    struct r_surface *surface,
    const rect_t * restrict src_rect,
    const rect_t * restrict dst_rect
);

void r_surface_destroy(struct r_surface **surface_p);

#endif /* R_SURFACE_H_ */
