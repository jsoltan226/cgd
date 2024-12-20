#include "rctx.h"
#include <core/log.h>
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

typedef void (blit_function_t)(
    const struct pixel_flat_data *restrict src_data,
    struct pixel_flat_data *restrict dst_data,
    const rect_t *src_rect,
    const rect_t *dst_rect,
    const f32 scale_x,
    const f32 scale_y,
    const pixelfmt_t dst_pixelfmt
);
static blit_function_t unscaled_unconverted_blit;
static blit_function_t unscaled_converted_blit;
static blit_function_t scaled_unconverted_blit;
static blit_function_t scaled_converted_blit;

struct r_surface * r_surface_create(u32 w, u32 h,
    pixelfmt_t color_format)
{
    struct pixel_flat_data pixel_data;
    pixel_data.w = w;
    pixel_data.h = h;
    pixel_data.buf = calloc(w * h, sizeof(pixel_t));
    if (pixel_data.buf == NULL)
        goto_error("Failed to allocate new surface (width: %u, height: %u).",
            w, h);

    return r_surface_init(&pixel_data, color_format);

err:
    if (pixel_data.buf != NULL) u_nfree(&pixel_data.buf);
    return NULL;
}

struct r_surface * r_surface_init(struct pixel_flat_data *pixels,
    pixelfmt_t color_format)
{
    u_check_params(pixels != NULL);

    struct r_surface *s = calloc(1, sizeof(struct r_surface));
    s_assert(s != NULL, "calloc() failed for new surface");

    s->color_format = color_format;
    s->data.w = pixels->w;
    s->data.h = pixels->h;
    s->data.buf = pixels->buf;
    u_rect_from_pixel_data(&s->data, &s->data_rect);

    return s;
}

void r_surface_blit(struct r_surface *dst, const struct r_surface *src,
    const rect_t *src_rect, const rect_t *dst_rect)
{
    u_check_params(src != NULL && dst != NULL);

    /* Handle src_rect and dst_rect being NULL */
    rect_t final_src_rect, final_dst_rect;
    memcpy(&final_src_rect,
        src_rect != NULL ? src_rect : &src->data_rect,
        sizeof(rect_t)
    );
    memcpy(&final_dst_rect,
        dst_rect != NULL ? dst_rect : &dst->data_rect,
        sizeof(rect_t)
    );

    /* The scale must be calculated before clipping the rects */
    const f32 scale_x = (f32)final_src_rect.w / final_dst_rect.w;
    const f32 scale_y = (f32)final_src_rect.h / final_dst_rect.h;
    
    /* Clip the rects to make sure we don't read or write out of bounds */
    rect_t tmp = final_dst_rect;
    rect_clip(&final_dst_rect, &dst->data_rect);

    /* "Project" the changes made to dst_rect onto src_rect */
    final_src_rect.x -= (tmp.x - final_dst_rect.x) * scale_x;
    final_src_rect.y -= (tmp.y - final_dst_rect.y) * scale_y;
    final_src_rect.w -= (tmp.w - final_dst_rect.w) * scale_x;
    final_src_rect.h -= (tmp.h - final_dst_rect.h) * scale_y;

    /* Again, ensure that we don't read out of bounds */
    rect_clip(&final_src_rect, &src->data_rect);

    /* Return early if there is nothing to do */
    if (final_src_rect.w == 0 || final_src_rect.h == 0 ||
        final_dst_rect.w == 0 || final_dst_rect.h == 0)
        return;

    /* Use faster blitting functions if we can */
    const u8 needs_pixel_conversion =
        (src->color_format != dst->color_format)
        << 0;

    const u8 needs_scaling =
        (final_src_rect.w != final_dst_rect.w ||
         final_src_rect.h != final_dst_rect.h)
        << 1;

    /* Fancy switch statement */
#define CONVERSION 1 << 0
#define SCALING 1 << 1
#define NO_CONVERSION 0
#define NO_SCALING 0
    static blit_function_t *const blit_function_table[4] = {
        [NO_SCALING | NO_CONVERSION]    = &unscaled_unconverted_blit,
        [NO_SCALING | CONVERSION]       = &unscaled_converted_blit,
        [SCALING | NO_CONVERSION]       = &scaled_unconverted_blit,
        [SCALING | CONVERSION]          = &scaled_converted_blit,
    };

    const u8 index = needs_scaling | needs_pixel_conversion;
    s_assert(index >= 0 && index < 4, "how?");

    (*(blit_function_table[index])) (
        &src->data, &dst->data,
        &final_src_rect, &final_dst_rect,
        scale_x, scale_y,
        dst->color_format
    );
}

void r_surface_render(struct r_ctx *rctx, const struct r_surface *src,
    const rect_t *src_rect, const rect_t *dst_rect)
{
    u_check_params(rctx != NULL && src != NULL);

    struct r_surface tmp_rctx_surface;
    memcpy(&tmp_rctx_surface.data, rctx->render_buffer,
        sizeof(struct pixel_flat_data));
    memcpy(&tmp_rctx_surface.data_rect, &rctx->pixels_rect, sizeof(rect_t));
    tmp_rctx_surface.color_format = rctx->win_meta.color_format;

    r_surface_blit(&tmp_rctx_surface, src, src_rect, dst_rect);
}

void r_surface_destroy(struct r_surface **surface_p)
{
    if (surface_p == NULL || *surface_p == NULL) return;
    struct r_surface *surface = *surface_p;

    if (surface->data.buf != NULL)
        u_nfree(&surface->data.buf);

    u_nzfree(surface_p);
}

static void unscaled_unconverted_blit(
    const struct pixel_flat_data *restrict src_data,
    struct pixel_flat_data *restrict dst_data,
    const rect_t *src_rect,
    const rect_t *dst_rect,
    const f32 scale_x,
    const f32 scale_y,
    const pixelfmt_t dst_pixelfmt
)
{
    (void) scale_x;
    (void) scale_y;
    (void) dst_pixelfmt;

    for (u32 y = dst_rect->y; y < dst_rect->y + dst_rect->h; y++) {
        const u32 dst_offset = ((dst_rect->y + y) * dst_data->w) + dst_rect->x;
        const u32 src_offset = ((src_rect->y + y) * src_data->w) + src_rect->x;

        memcpy(
            dst_data->buf + dst_offset,
            src_data->buf + src_offset,
            dst_rect->w * sizeof(pixel_t)
        );
    }
}

static void unscaled_converted_blit(
    const struct pixel_flat_data *restrict src_data,
    struct pixel_flat_data *restrict dst_data,
    const rect_t *src_rect,
    const rect_t *dst_rect,
    const f32 scale_x,
    const f32 scale_y,
    const pixelfmt_t dst_pixelfmt
)
{
    (void) scale_x;
    (void) scale_y;

    for (u32 dy = 0; dy < dst_rect->h; dy++) {
        for (u32 dx = 0; dx < dst_rect->w; dx++) {
            const u32 src_offset =
                (src_data->w * (src_rect->y + dy)) + (src_rect->x + dx);

            r_putpixel_fast_(
                dst_data->buf,
                dst_rect->x + dx,
                dst_rect->y + dy,
                dst_data->w,
                *(src_data->buf + src_offset),
                dst_pixelfmt
            );
        }
    }
}

static void scaled_unconverted_blit(
    const struct pixel_flat_data *restrict src_data,
    struct pixel_flat_data *restrict dst_data,
    const rect_t *src_rect,
    const rect_t *dst_rect,
    const f32 scale_x,
    const f32 scale_y,
    const pixelfmt_t dst_pixelfmt
)
{
    (void) dst_pixelfmt;

    f32 sx = (f32)src_rect->x;
    f32 sy = (f32)src_rect->y;

    for (i32 y = dst_rect->y; y < dst_rect->y + dst_rect->h; y++) {
        for (i32 x = dst_rect->x; x < dst_rect->x + dst_rect->w; x++) {
            const pixel_t src_pixel =
                *(src_data->buf + ((i32)sy * src_data->w) + (i32)sx);

            r_putpixel_fast_matching_pixelfmt_(
                dst_data->buf,
                x, y, dst_data->w,
                src_pixel
            );
            sx += scale_x;
        }
        sx = 0;
        sy += scale_y;
    }
}

static void scaled_converted_blit(
    const struct pixel_flat_data *restrict src_data,
    struct pixel_flat_data *restrict dst_data,
    const rect_t *src_rect,
    const rect_t *dst_rect,
    const f32 scale_x,
    const f32 scale_y,
    const pixelfmt_t dst_pixelfmt
)
{
    f32 sx = src_rect->x;
    f32 sy = src_rect->y;
    for (i32 y = dst_rect->y; y < dst_rect->y + dst_rect->h; y++) {
        for (i32 x = dst_rect->x; x < dst_rect->x + dst_rect->w; x++) {
            const pixel_t src_pixel = *(
                src_data->buf
                + ((src_rect->y + (i32)sy) * src_data->w)
                + (src_rect->x + (i32)sx)
            );

            r_putpixel_fast_(
                dst_data->buf,
                x, y, dst_data->w,
                src_pixel,
                dst_pixelfmt
            );
            sx += scale_x;
        }
        sx = 0;
        sy += scale_y;
    }
}
