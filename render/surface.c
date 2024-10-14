#include "core/log.h"
#include <core/int.h>
#include <core/util.h>
#include <core/math.h>
#include <core/pixel.h>
#include <core/shapes.h>
#include <stdlib.h>
#include <string.h>
#define R_INTERNAL_GUARD__
#include "surface.h"
#undef R_INTERNAL_GUARD__
#define R_INTERNAL_GUARD__
#include "rctx-internal.h"
#undef R_INTERNAL_GUARD__
#define R_INTERNAL_GUARD__
#include "putpixel-fast.h"
#undef R_INTERNAL_GUARD__

#define MODULE_NAME "surface"

static void memcpy_blit(
    const struct pixel_flat_data * restrict data,
    struct r_ctx *rctx,
    const rect_t * restrict src_rect,
    const rect_t * restrict dst_rect
);
static void scale_matching_format_blit(
    const struct pixel_flat_data * restrict data,
    struct r_ctx *rctx,
    const rect_t * restrict src_rect,
    const rect_t * restrict dst_rect
);
static void scale_normal_blit(
    const struct pixel_flat_data * restrict data,
    struct r_ctx *rctx,
    const rect_t * restrict src_rect,
    const rect_t * restrict dst_rect
);

struct r_surface * r_surface_create(struct r_ctx *rctx, u32 w, u32 h,
    enum p_window_color_type color_format)
{
    u_check_params(rctx != NULL);

    struct pixel_flat_data pixel_data;
    pixel_data.w = w;
    pixel_data.h = h;
    pixel_data.buf = calloc(w * h, sizeof(pixel_t));
    if (pixel_data.buf == NULL)
        goto_error("Failed to allocate new surface (width: %u, height: %u).",
            w, h);

    struct r_surface *ret = r_surface_init(rctx, &pixel_data, color_format);
    return ret;

err:
    if (pixel_data.buf != NULL) free(pixel_data.buf);
    return NULL;
}

struct r_surface * r_surface_init(struct r_ctx *rctx,
    struct pixel_flat_data *pixels, enum p_window_color_type color_format)
{
    u_check_params(rctx != NULL && pixels != NULL);

    struct r_surface *s = calloc(1, sizeof(struct r_surface));
    s_assert(s != NULL, "calloc() failed for new surface");

    s->rctx = rctx;
    s->color_format = color_format;
    s->data.w = pixels->w;
    s->data.h = pixels->h;
    s->data.buf = pixels->buf;

    return s;
}

void r_surface_blit(struct r_surface *surface,
    const rect_t * restrict src_rect, const rect_t * restrict dst_rect)
{
    u_check_params(surface != NULL);

    const rect_t *final_src_rect = src_rect != NULL ? src_rect :
        &(rect_t){ 0, 0, surface->data.w, surface->data.h };

    const rect_t *final_dst_rect = dst_rect != NULL ? dst_rect :
        &(rect_t){ 0, 0, surface->data.w, surface->data.h };

    /* Return early if there is nothing to draw */
    if ((final_src_rect && 
            (final_src_rect->w == 0 || final_src_rect->h == 0)) ||
        (final_dst_rect && 
            (final_dst_rect->w == 0 || final_dst_rect->h == 0))
    )
        return;

    const bool pixel_format_matches =
        surface->color_format == surface->rctx->win_meta.color_type;

    const bool scale_matches =
        (final_src_rect->w == final_dst_rect->w) &&
        (final_src_rect->h == final_dst_rect->h);

    if (scale_matches && pixel_format_matches) {
        memcpy_blit(&surface->data, surface->rctx,
            final_src_rect, final_dst_rect);
    } else if (!scale_matches && pixel_format_matches) {
        scale_matching_format_blit(&surface->data, surface->rctx,
            final_src_rect, final_dst_rect);
    } else {
        scale_normal_blit(&surface->data, surface->rctx,
            final_src_rect, final_dst_rect);
    }
}

void r_surface_destroy(struct r_surface *surface)
{
    if (surface == NULL) return;

    if (surface->data.buf != NULL) {
        free(surface->data.buf);
    }

    memset(surface, 0, sizeof(struct r_surface));
    free(surface);
}

static void memcpy_blit(
    const struct pixel_flat_data * restrict data,
    struct r_ctx *rctx,
    const rect_t * restrict src_rect,
    const rect_t * restrict dst_rect
)
{
    for (u32 y = 0; y < src_rect->h; y++) {
        const u32 offset = ((dst_rect->y + y) * rctx->pixels.w) + dst_rect->x;
        memcpy(rctx->pixels.buf + offset, data->buf + y * data->w,
            u_min(data->w, rctx->pixels.w) * sizeof(pixel_t));
    }
}

static void scale_matching_format_blit(
    const struct pixel_flat_data * restrict data,
    struct r_ctx *rctx,
    const rect_t * restrict src_rect,
    const rect_t * restrict dst_rect
)
{
    const f32 scale_x = (f32)dst_rect->w / src_rect->w;
    const f32 scale_y = (f32)dst_rect->h / dst_rect->h;

    for (u32 dy = 0; dy < dst_rect->h; dy++) {
        for (u32 dx = 0; dx < dst_rect->w; dx++) {

            /* Calculate the nearest neighbouring pixel's position */
            const i32 sx = src_rect->x + (i32)(dx / scale_x);
            const i32 sy = src_rect->y + (i32)(dy / scale_y);

            const pixel_t src_pixel = *(data->buf + (sy * data->w) + sx);

            r_putpixel_fast_matching_pixelfmt_(
                rctx->pixels.buf,
                src_rect->x + dx,
                src_rect->y + dy,
                rctx->pixels.w,
                src_pixel
            );
        }
    }
}

static void scale_normal_blit(
    const struct pixel_flat_data * restrict data,
    struct r_ctx *rctx,
    const rect_t * restrict src_rect,
    const rect_t * restrict dst_rect
)
{
    const f32 scale_x = (f32)dst_rect->w / src_rect->w;
    const f32 scale_y = (f32)dst_rect->h / dst_rect->h;

    for (u32 dy = 0; dy < dst_rect->h; dy++) {
        for (u32 dx = 0; dx < dst_rect->w; dx++) {

            /* Calculate the nearest neighbouring pixel's position */
            const i32 sx = src_rect->x + (i32)(dx / scale_x);
            const i32 sy = src_rect->y + (i32)(dy / scale_y);

            const pixel_t src_pixel = *(data->buf + (sy * data->w) + sx);

            r_putpixel_fast_(
                rctx->pixels.buf,
                src_rect->x + dx,
                src_rect->y + dy,
                rctx->pixels.w,
                src_pixel, rctx->win_meta.color_type
            );
        }
    }
}
