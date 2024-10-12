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
#include "rctx_internal.h"
#undef R_INTERNAL_GUARD__

#define MODULE_NAME "surface"

static void memcpy_blit(struct pixel_flat_data *data, struct r_ctx *rctx,
    const rect_t *src_rect, const rect_t *dst_rect);

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
    const rect_t *src_rect, const rect_t *dst_rect)
{
    u_check_params(surface != NULL);

    const rect_t *final_src_rect = src_rect != NULL ? src_rect :
        &(rect_t){ 0, 0, surface->data.w, surface->data.h };

    const rect_t *final_dst_rect = dst_rect != NULL ? dst_rect :
        &(rect_t){ 0, 0, surface->data.w, surface->data.h };

    if (surface->color_format == surface->rctx->win_meta.color_type) {
        memcpy_blit(&surface->data, surface->rctx,
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

static void memcpy_blit(struct pixel_flat_data *data, struct r_ctx *rctx,
    const rect_t *src_rect, const rect_t *dst_rect)
{
    for (u32 y = 0; y < src_rect->h; y++) {
        const u32 offset = ((dst_rect->y + y) * rctx->pixels.w) + dst_rect->x;
        memcpy(rctx->pixels.buf + offset, data->buf + y * data->w,
            u_min(data->w, rctx->pixels.w) * sizeof(pixel_t));
    }
}
