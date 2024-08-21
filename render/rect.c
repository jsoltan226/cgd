#include "rect.h"
#include "core/int.h"
#include "core/math.h"
#include "putpixel.h"
#include <stdlib.h>

#define R_INTERNAL_GUARD__
#include "rctx_internal.h"
#undef R_INTERNAL_GUARD__

void r_draw_rect(struct r_ctx *ctx, rect_t *r)
{
    if (ctx == NULL || r == NULL) return;
    
    u32 start_x = max(0, min(r->x, ctx->pixels.w - 1));
    u32 end_x = max(0, min(r->x + r->w, ctx->pixels.w - 1));
    u32 start_y = max(0, min(r->y, ctx->pixels.h - 1));
    u32 end_y = max(0, min(r->y + r->h, ctx->pixels.h - 1));

    if (start_x == end_x || start_y == end_y)
        return;

    if (start_y == r->y) {
        for (u32 x = start_x; x < end_x; x++) {
            r_putpixel(ctx, x, start_y, ctx->current_color);
        }
    }
    if (end_y == r->y + r->h) {
        for (u32 x = start_x; x < end_x; x++) {
            r_putpixel(ctx, x, end_y, ctx->current_color);
        }
    }

    if (start_x == r->x) {
        for (u32 y = start_y + 1; y < end_y; y++) {
            r_putpixel(ctx, start_x, y, ctx->current_color);
        }
    }
    if (end_x == r->x + r->w) {
        for (u32 y = start_y + 1; y < end_y; y++) {
            r_putpixel(ctx, end_x - 1, y, ctx->current_color);
        }
    }
}

void r_fill_rect(struct r_ctx *ctx, rect_t *r)
{
    
    if (ctx == NULL || r == NULL) return;
    
    u32 start_x = min(r->x, ctx->pixels.w - 1);
    u32 end_x = min(r->x + r->w, ctx->pixels.w - 1);
    u32 start_y = min(r->y, ctx->pixels.h - 1);
    u32 end_y = min(r->y + r->h, ctx->pixels.h - 1);

    if (start_x == end_x || start_y == end_y)
        return;

    for (u32 y = start_y; y < end_y; y++) {
        for (u32 x = start_x; x < end_x; x++) {
            r_putpixel(ctx, x, y, ctx->current_color);
        }
    }
}
