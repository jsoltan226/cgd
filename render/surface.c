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
static blit_function_t unscaled_unconverted_alpha_blit;
static blit_function_t unscaled_converted_alpha_blit;
static blit_function_t scaled_unconverted_alpha_blit;
static blit_function_t scaled_converted_alpha_blit;
static blit_function_t unscaled_unconverted_noalpha_blit;
static blit_function_t unscaled_converted_noalpha_blit;
static blit_function_t scaled_unconverted_noalpha_blit;
static blit_function_t scaled_converted_noalpha_blit;

struct r_surface * r_surface_create(u32 w, u32 h,
    pixelfmt_t color_format)
{
    if (color_format == RGB24 || color_format == BGR24) {
        s_log_error("24-bit surfaces are not yet supported!");
        return NULL;
    }

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

    if (color_format == RGB24 || color_format == BGR24) {
        s_log_error("24-bit surfaces are not yet supported!");
        return NULL;
    }

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
    if (src->color_format == RGB24 || src->color_format == BGR24 ||
        dst->color_format == RGB24 || dst->color_format == BGR24)
        s_log_fatal(MODULE_NAME, __func__,
            "24-bit surfaces are not yet supported!");

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
    const f32 scale_x = (f32)final_src_rect.w / (f32)final_dst_rect.w;
    const f32 scale_y = (f32)final_src_rect.h / (f32)final_dst_rect.h;
    
    /* Clip the rects to make sure we don't read or write out of bounds */
    rect_t tmp = final_dst_rect;
    rect_clip(&final_dst_rect, &dst->data_rect);

    /* "Project" the changes made to dst_rect onto src_rect */
    final_src_rect.x -= (f32)(tmp.x - final_dst_rect.x) * scale_x;
    final_src_rect.y -= (f32)(tmp.y - final_dst_rect.y) * scale_y;
    final_src_rect.w -= (f32)(tmp.w - final_dst_rect.w) * scale_x;
    final_src_rect.h -= (f32)(tmp.h - final_dst_rect.h) * scale_y;

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

    /* If both src or dst have a pixel format that ignores
     * the alpha channel, the blit can ingore it */
    const u8 uses_alpha = (1 || !(
            (src->color_format == RGBX32 || src->color_format == BGRX32) &&
            (dst->color_format == RGBX32 || dst->color_format == BGRX32)
        )) << 2;

    /* Fancy switch statement */
#define CONV    1 << 0
#define SCALE   1 << 1
#define ALPHA   1 << 2
#define NO_CONV     0 << 0
#define NO_SCALE    0 << 1
#define NO_ALPHA    0 << 2
    static blit_function_t *const blit_function_table[8] = {
        [NO_SCALE | NO_CONV | NO_ALPHA  ] = &unscaled_unconverted_noalpha_blit,
        [NO_SCALE | NO_CONV | ALPHA     ] = &unscaled_unconverted_alpha_blit,
        [NO_SCALE | CONV    | NO_ALPHA  ] = &unscaled_converted_noalpha_blit,
        [NO_SCALE | CONV    | ALPHA     ] = &unscaled_converted_alpha_blit,
        [SCALE    | NO_CONV | NO_ALPHA  ] = &scaled_unconverted_noalpha_blit,
        [SCALE    | NO_CONV | ALPHA     ] = &scaled_unconverted_alpha_blit,
        [SCALE    | CONV    | NO_ALPHA  ] = &scaled_converted_noalpha_blit,
        [SCALE    | CONV    | ALPHA     ] = &scaled_converted_alpha_blit,
    };

    const u8 index = needs_scaling | needs_pixel_conversion | uses_alpha;
    s_assert(index >= 0 && index < 8, "how?");

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

    /* Don't ignore the alpha channel since we are not drawing
     * to a normal surface, but to the whole framebuffer,
     * which is where everything else also gets rendered */
    tmp_rctx_surface.color_format = rctx->win_meta.color_format;
    if (tmp_rctx_surface.color_format == BGRX32)
        tmp_rctx_surface.color_format = BGRA32;
    else if (tmp_rctx_surface.color_format == RGBX32)
        tmp_rctx_surface.color_format = RGBA32;

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

static void unscaled_unconverted_alpha_blit(
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
    for (u32 dy = 0; dy < dst_rect->h; dy++) {
        for (u32 dx = 0; dx < dst_rect->w; dx++) {
            const u32 src_offset =
                (src_data->w * (src_rect->y + dy)) + (src_rect->x + dx);

            r_putpixel_fast_matching_pixelfmt_(
                dst_data->buf,
                dst_rect->x + dx,
                dst_rect->y + dy,
                dst_data->w,
                *(src_data->buf + src_offset)
            );
        }
    }
}

static void unscaled_unconverted_noalpha_blit(
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

static void unscaled_converted_alpha_blit(
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

static void unscaled_converted_noalpha_blit(
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

            r_putpixel_fast_noalpha_(
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

static void scaled_unconverted_alpha_blit(
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
            const pixel_t src_pixel = *(
                src_data->buf
                + ((src_rect->y + (i32)sy) * src_data->w)
                + (src_rect->x + (i32)sx)
            );

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

static void scaled_unconverted_noalpha_blit(
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
            const pixel_t src_pixel = *(
                src_data->buf
                + ((src_rect->y + (i32)sy) * src_data->w)
                + (src_rect->x + (i32)sx)
            );

            r_putpixel_fast_matching_pixelfmt_noalpha_(
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

static void scaled_converted_alpha_blit(
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

static void scaled_converted_noalpha_blit(
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

            r_putpixel_fast_noalpha_(
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
