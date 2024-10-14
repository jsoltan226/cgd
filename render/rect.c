#include "rect.h"
#include <core/int.h>
#include <core/math.h>
#include <core/util.h>
#include <core/pixel.h>
#include <core/shapes.h>
#include <platform/window.h>
#include <SDL2/SDL_pixels.h>
#include <stdlib.h>
#define R_INTERNAL_GUARD__
#include "rctx-internal.h"
#undef R_INTERNAL_GUARD__
#define R_INTERNAL_GUARD__
#include "putpixel-fast.h"
#undef R_INTERNAL_GUARD__

/* "u_" for "user-provided" */
void r_draw_rect(struct r_ctx *ctx,
    const i32 u_x, const i32 u_y,
    const i32 u_w, const i32 u_h)
{
    if (ctx == NULL || u_w < 0 || u_h < 0) return;
    
    const u32 start_x = u_clamp(0, u_x,        ctx->pixels.w - 1);
    const u32 end_x   = u_clamp(0, u_x + u_w,  ctx->pixels.w - 1);
    const u32 start_y = u_clamp(0, u_y,        ctx->pixels.h - 1);
    const u32 end_y   = u_clamp(0, u_y + u_h,  ctx->pixels.h - 1);

    if (start_x == end_x || start_y == end_y)
        return;

    register pixel_t *buf = ctx->pixels.buf;
    register color_RGBA32_t color = ctx->current_color;
    register u32 x, y;
    register u32 w = ctx->pixels.w;

    if (start_y == u_y) {
        y = start_y;
        for (x = start_x; x < end_x; x++)
            r_putpixel_fast_matching_pixelfmt_(buf, x, y, w, color);
    }

    if (end_y == u_y + u_h) {
        y = end_y;
        for (u32 x = start_x; x < end_x; x++)
            r_putpixel_fast_matching_pixelfmt_(buf, x, y, w, color);
    }

    if (start_x == u_x) {
        x = start_x;
        for (y = start_y + 1; y < end_y; y++)
            r_putpixel_fast_matching_pixelfmt_(buf, x, y, w, color);
    }

    if (end_x == u_x + u_w) {
        x = end_x - 1;
        for (y = start_y + 1; y < end_y; y++)
            r_putpixel_fast_matching_pixelfmt_(buf, x, y, w, color);
    }
}

/* "u_" for "user-provided" */
void r_fill_rect(struct r_ctx *ctx,
    const i32 u_x, const i32 u_y,
    const i32 u_w, const i32 u_h)
{
    
    if (ctx == NULL || u_w < 0 || u_h < 0) return;
    
    const u32 start_x = u_clamp(0, u_x,        ctx->pixels.w - 1);
    const u32 end_x   = u_clamp(0, u_x + u_w,  ctx->pixels.w - 1);
    const u32 start_y = u_clamp(0, u_y,        ctx->pixels.h - 1);
    const u32 end_y   = u_clamp(0, u_y + u_h,  ctx->pixels.h - 1);

    if (start_x == end_x || start_y == end_y)
        return;

    register pixel_t *buf = ctx->pixels.buf;
    register u32 w = ctx->pixels.w;
    register color_RGBA32_t color = ctx->current_color;

    for (register u32 y = start_y; y < end_y; y++) {
        for (register u32 x = start_x; x < end_x; x++) {
            r_putpixel_fast_matching_pixelfmt_(buf, x, y, w, color);
        }
    }
}
