#ifndef R_PUTPIXEL_FAST__
#define R_PUTPIXEL_FAST__

#include "core/pixel.h"
#include "core/shapes.h"
#include "platform/window.h"
#ifndef R_INTERNAL_GUARD__
#error This header is internal to the cgd renderer module and is not intented to be used elsewhere
#endif /* R_INTERNAL_GUARD__ */

static inline void r_putpixel_fast_(
    pixel_t *buf, i32 x, i32 y, u32 w, register color_RGBA32_t color,
    enum p_window_color_type color_type)
{
    if (color_type == P_WINDOW_BGRA8888) {
        register u8 tmp = color.r;
        color.r = color.b;
        color.b = tmp;
    }

    *(buf + ((y * w) + x)) = color;
}

#endif /* R_PUTPIXEL_FAST__ */
