#ifndef R_PUTPIXEL_FAST__
#define R_PUTPIXEL_FAST__

#include <core/util.h>
#include <core/pixel.h>
#include <core/shapes.h>
#include <platform/window.h>
#ifndef R_INTERNAL_GUARD__
#error This header is internal to the cgd renderer module and is not intented to be used elsewhere
#endif /* R_INTERNAL_GUARD__ */


#define r_putpixel_fast_(                           \
    /* pixel_t * */ buf_,                           \
    /* i32 */ x_, /* i32 */ y_, /* u32 */ stride_,  \
    /* color_RGBA32_t */ color_,                    \
    /* pixelfmt_t */ dst_pixelfmt_                  \
) do {                                                                      \
    u_macro_type_check(r_putpixel_fast_, pixel_t *, (buf_));                \
    u_macro_type_check(r_putpixel_fast_, i32, (x_));                        \
    u_macro_type_check(r_putpixel_fast_, i32, (y_));                        \
    u_macro_type_check(r_putpixel_fast_, u32, (stride_));                   \
    u_macro_type_check(r_putpixel_fast_, color_RGBA32_t, (color_));         \
    u_macro_type_check(r_putpixel_fast_, pixelfmt_t, (dst_pixelfmt_));      \
                                                                            \
    /* Get the pixel location in the buffer */                              \
    pixel_t *const pixel_ptr = (buf_) + (((y_) * (stride_)) + (x_));        \
                                                                            \
    color_RGBA32_t final_color = (color_);                                  \
                                                                            \
    /* Convert BGRA if needed */                                            \
    if ((dst_pixelfmt_) == BGRA32) {                                        \
        register u8 tmp = final_color.r;                                    \
        final_color.r = final_color.b;                                      \
        final_color.b = tmp;                                                \
    }                                                                       \
                                                                            \
    const u8 src_alpha = final_color.a;                                     \
    if (src_alpha == 255) {                                                 \
        /* Fully opaque, just write the new color */                        \
        *pixel_ptr = final_color;                                           \
    } else if (src_alpha > 0) {                                             \
        /* Perform alpha blending */                                        \
                                                                            \
        /* Read the existing color */                                       \
        color_RGBA32_t dst = *pixel_ptr;                                    \
                                                                            \
        /* Blend each final_color channel */                                \
        const u8 inv_alpha = 255 - src_alpha;                               \
                                                                            \
        dst.r = (final_color.r * src_alpha + dst.r * inv_alpha) / 255;      \
        dst.g = (final_color.g * src_alpha + dst.g * inv_alpha) / 255;      \
        dst.b = (final_color.b * src_alpha + dst.b * inv_alpha) / 255;      \
                                                                            \
        /* Write the blended color back */                                  \
        *pixel_ptr = dst;                                                   \
    }                                                                       \
} while (0)

#define r_putpixel_fast_noalpha_(                   \
    /* pixel_t * */ buf_,                           \
    /* i32 */ x_, /* i32 */ y_, /* u32 */ stride_,  \
    /* color_RGBA32_t */ color_,                    \
    /* pixelfmt_t */ dst_pixelfmt_                  \
) do {                                                                      \
    u_macro_type_check(r_putpixel_fast_noalpha_, pixel_t *, (buf_));        \
    u_macro_type_check(r_putpixel_fast_noalpha_, i32, (x_));                \
    u_macro_type_check(r_putpixel_fast_noalpha_, i32, (y_));                \
    u_macro_type_check(r_putpixel_fast_noalpha_, u32, (stride_));           \
    u_macro_type_check(r_putpixel_fast_noalpha_, color_RGBA32_t, (color_)); \
    u_macro_type_check(r_putpixel_fast_noalpha_,                            \
        pixelfmt_t, (dst_pixelfmt_));                                       \
                                                                            \
    color_RGBA32_t final_color = (color_);                                  \
                                                                            \
    if ((dst_pixelfmt_) == BGRA32) {                                        \
        register u8 tmp = final_color.r;                                    \
        final_color.r = final_color.b;                                      \
        final_color.b = tmp;                                                \
    }                                                                       \
                                                                            \
    *((buf_) + (((y_) * (stride_)) + (x_))) = final_color;                  \
} while (0)                                                                 \

#define r_putpixel_fast_matching_pixelfmt_(         \
    /* pixel_t * */ buf_,                           \
    /* i32 */ x_, /* i32 */ y_, /* u32 */ stride_,  \
    /* color_RGBA32_t */ color_                     \
) do {                                                                      \
    u_macro_type_check(r_putpixel_fast_matching_pixelfmt_,                  \
        pixel_t *, (buf_));                                                 \
    u_macro_type_check(r_putpixel_fast_matching_pixelfmt_, i32, (x_));      \
    u_macro_type_check(r_putpixel_fast_matching_pixelfmt_, i32, (y_));      \
    u_macro_type_check(r_putpixel_fast_matching_pixelfmt_, u32, (stride_)); \
    u_macro_type_check(r_putpixel_fast_matching_pixelfmt_,                  \
        color_RGBA32_t, (color_));                                          \
                                                                            \
    register pixel_t *const pixel_ptr = (buf_) + ((y_) * (stride_)) + (x_); \
                                                                            \
    /* Read the existing color */                                           \
    register color_RGBA32_t dst = *pixel_ptr;                               \
                                                                            \
    /* Blend each color channel */                                          \
    register const u8 inv_alpha = 255 - (color_).a;                         \
                                                                            \
    dst.r = ((color_).r * (color_).a + dst.r * inv_alpha) / 255;            \
    dst.g = ((color_).g * (color_).a + dst.g * inv_alpha) / 255;            \
    dst.b = ((color_).b * (color_).a + dst.b * inv_alpha) / 255;            \
                                                                            \
    /* Write the blended color back */                                      \
    *pixel_ptr = dst;                                                       \
} while (0)

#define r_putpixel_fast_matching_pixelfmt_noalpha_( \
    /* pixel_t * */ buf_,                           \
    /* i32 */ x_, /* i32 */ y_, /* u32 */ stride_,  \
    /* color_RGBA32_t */ color_                     \
) do {                                                                      \
    u_macro_type_check(r_putpixel_fast_matching_pixelfmt_noalpha_,          \
        pixel_t *, buf_);                                                   \
    u_macro_type_check(r_putpixel_fast_matching_pixelfmt_noalpha_,          \
        i32, x_);                                                           \
    u_macro_type_check(r_putpixel_fast_matching_pixelfmt_noalpha_,          \
        i32, y_);                                                           \
    u_macro_type_check(r_putpixel_fast_matching_pixelfmt_noalpha_,          \
        u32, stride_);                                                      \
    u_macro_type_check(r_putpixel_fast_matching_pixelfmt_noalpha_,          \
        color_RGBA32_t, color_);                                            \
                                                                            \
    *((buf_) + (((y_) * (stride_)) + (x_))) = (color_);                     \
} while (0)

#endif /* R_PUTPIXEL_FAST__ */
