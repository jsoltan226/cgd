#include "rect.h"
#include "core/int.h"
#include "core/math.h"
#include "core/pixel.h"
#include "core/shapes.h"
#include "platform/window.h"
#include <SDL2/SDL_pixels.h>
#include <stdlib.h>

#define R_INTERNAL_GUARD__
#include "rctx_internal.h"
#undef R_INTERNAL_GUARD__
#define R_INTERNAL_GUARD__
#include "putpixel_fast.h"
#undef R_INTERNAL_GUARD__

void r_draw_rect(struct r_ctx *ctx, const rect_t *r)
{
    if (ctx == NULL || r == NULL) return;
    
    u32 start_x = u_clamp(r->x,         0, ctx->pixels.w - 1);
    u32 end_x   = u_clamp(r->x + r->w,  0, ctx->pixels.w - 1);
    u32 start_y = u_clamp(r->y,         0, ctx->pixels.h - 1);
    u32 end_y   = u_clamp(r->y + r->h,  0, ctx->pixels.h - 1);

    if (start_x == end_x || start_y == end_y)
        return;

    register pixel_t *buf = ctx->pixels.buf;
    register color_RGBA32_t color = ctx->current_color;
    register enum p_window_color_type color_type = ctx->win_meta.color_type;
    register u32 x, y;
    register u32 w = ctx->pixels.w;

    if (start_y == r->y) {
        y = start_y;
        for (x = start_x; x < end_x; x++)
            r_putpixel_fast_(buf, x, y, w, color, color_type);
    }

    if (end_y == r->y + r->h) {
        y = end_y;
        for (u32 x = start_x; x < end_x; x++)
            r_putpixel_fast_(buf, x, y, w, color, color_type);
    }

    if (start_x == r->x) {
        x = start_x;
        for (y = start_y + 1; y < end_y; y++)
            r_putpixel_fast_(buf, x, y, w, color, color_type);
    }

    if (end_x == r->x + r->w) {
        x = end_x - 1;
        for (y = start_y + 1; y < end_y; y++)
            r_putpixel_fast_(buf, x, y, w, color, color_type);
    }
}

void r_fill_rect(struct r_ctx *ctx, const rect_t *r)
{
    
    if (ctx == NULL || r == NULL) return;
    
    u32 start_x = u_min(r->x,           ctx->pixels.w - 1);
    u32 end_x   = u_min(r->x + r->w,    ctx->pixels.w - 1);
    u32 start_y = u_min(r->y,           ctx->pixels.h - 1);
    u32 end_y   = u_min(r->y + r->h,    ctx->pixels.h - 1);

    if (start_x == end_x || start_y == end_y)
        return;

    register pixel_t *buf = ctx->pixels.buf;
    register u32 w = ctx->pixels.w;
    register color_RGBA32_t color = ctx->current_color;
    register enum p_window_color_type color_type = ctx->win_meta.color_type;

    for (register u32 y = start_y; y < end_y; y++) {
        for (register u32 x = start_x; x < end_x; x++) {
            r_putpixel_fast_(buf, x, y, w, color, color_type);
        }
    }
}
