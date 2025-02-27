#ifndef R_PUTPIXEL_FAST__
#define R_PUTPIXEL_FAST__

#include <core/pixel.h>
#include <core/shapes.h>
#include <platform/window.h>
#ifndef R_INTERNAL_GUARD__
#error This header is internal to the cgd renderer module and is not intented to be used elsewhere
#endif /* R_INTERNAL_GUARD__ */

static inline void r_putpixel_fast_(
    pixel_t *const buf,
    const i32 x, const i32 y, const u32 stride,
    register color_RGBA32_t color,
    const pixelfmt_t dst_pixelfmt)
{
    /* Get the pixel location in the buffer */
    pixel_t *const pixel_ptr = buf + ((y * stride) + x);

    /* Convert BGRA if needed */
    if (dst_pixelfmt == BGRA32) {
        register u8 tmp = color.r;
        color.r = color.b;
        color.b = tmp;
    }

    register u8 src_alpha = color.a;
    if (src_alpha == 255) {
        /* Fully opaque, just write the new color */
        *pixel_ptr = color;
    } else if (src_alpha > 0) {
        /* Perform alpha blending */

        /* Read the existing color */
        color_RGBA32_t dst = *pixel_ptr;

        /* Blend each color channel */
        const u8 inv_alpha = 255 - src_alpha;

        dst.r = (u8)((color.r * src_alpha + dst.r * inv_alpha) / 255);
        dst.g = (u8)((color.g * src_alpha + dst.g * inv_alpha) / 255);
        dst.b = (u8)((color.b * src_alpha + dst.b * inv_alpha) / 255);

        /* Write the blended color back */
        *pixel_ptr = dst;
    }
}

static inline void r_putpixel_fast_noalpha_(
    pixel_t *const buf,
    const i32 x, const i32 y, const u32 stride,
    register color_RGBA32_t color,
    const pixelfmt_t dst_pixelfmt)
{
    if (dst_pixelfmt == BGRA32) {
        register u8 tmp = color.r;
        color.r = color.b;
        color.b = tmp;
    }

    *(buf + ((y * stride) + x)) = color;
}

static inline void r_putpixel_fast_matching_pixelfmt_(
    pixel_t *const buf,
    const i32 x, const i32 y, const u32 stride,
    const color_RGBA32_t color)
{

    register pixel_t *const pixel_ptr = buf + (y * stride) + x;

    /* Read the existing color */
    register color_RGBA32_t dst = *pixel_ptr;

    /* Blend each color channel */
    register const u8 inv_alpha = 255 - color.a;

    dst.r = (u8)((color.r * color.a + dst.r * inv_alpha) / 255);
    dst.g = (u8)((color.g * color.a + dst.g * inv_alpha) / 255);
    dst.b = (u8)((color.b * color.a + dst.b * inv_alpha) / 255);

    /* Write the blended color back */
    *pixel_ptr = dst;
}

static inline void r_putpixel_fast_matching_pixelfmt_noalpha_(
    pixel_t *const buf,
    const i32 x, const i32 y, const u32 stride,
    const color_RGBA32_t color)
{
    *(buf + ((y * stride) + x)) = color;
}

#endif /* R_PUTPIXEL_FAST__ */
