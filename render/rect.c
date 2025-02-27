#include "rect.h"
#include <core/int.h>
#include <core/math.h>
#include <core/util.h>
#include <core/pixel.h>
#include <core/shapes.h>
#include <platform/window.h>
#include <stdlib.h>
#include <limits.h>
#define R_INTERNAL_GUARD__
#include "rctx-internal.h"
#undef R_INTERNAL_GUARD__
#define R_INTERNAL_GUARD__
#include "putpixel-fast.h"
#undef R_INTERNAL_GUARD__

void r_draw_rect(struct r_ctx *ctx,
    const i32 x, const i32 y, const i32 w, const i32 h)
{
    if (ctx == NULL || w < 0 || h < 0 ||
        ctx->curr_buf->w > INT_MAX || ctx->curr_buf->h > INT_MAX ||
        ctx->curr_buf->w == 0 || ctx->curr_buf->h == 0)
        return;

    const i32 start_x = u_clamp(0, x,       (i32)ctx->curr_buf->w - 1);
    const i32 end_x   = u_clamp(0, x + w,   (i32)ctx->curr_buf->w - 1);
    const i32 start_y = u_clamp(0, y,       (i32)ctx->curr_buf->h - 1);
    const i32 end_y   = u_clamp(0, y + h,   (i32)ctx->curr_buf->h - 1);

    if (start_x == end_x || start_y == end_y)
        return;

    register pixel_t *const buf = ctx->curr_buf->buf;
    register const color_RGBA32_t color = ctx->current_color;
    register const u32 stride = ctx->curr_buf->w;
    register i32 x_, y_;

    if (start_y == y) {
        y_ = start_y;
        for (x_ = start_x; x_ < end_x; x_++)
            r_putpixel_fast_matching_pixelfmt_(buf, x_, y_, stride, color);
    }

    if (end_y == y + h) {
        y_ = end_y;
        for (x_ = start_x; x_ < end_x; x_++)
            r_putpixel_fast_matching_pixelfmt_(buf, x_, y_, stride, color);
    }

    if (start_x == x) {
        x_ = start_x;
        for (y_ = start_y + 1; y_ < end_y; y_++)
            r_putpixel_fast_matching_pixelfmt_(buf, x_, y_, stride, color);
    }

    if (end_x == x + w) {
        x_ = end_x - 1;
        for (y_ = start_y + 1; y_ < end_y; y_++)
            r_putpixel_fast_matching_pixelfmt_(buf, x_, y_, stride, color);
    }
}

/* "u_" for "user-provided" */
void r_fill_rect(struct r_ctx *ctx,
    const i32 x, const i32 y,
    const i32 w, const i32 h)
{

    if (ctx == NULL || w < 0 || h < 0 ||
        ctx->curr_buf->w > INT_MAX || ctx->curr_buf->h > INT_MAX ||
        ctx->curr_buf->w == 0 || ctx->curr_buf->h == 0)
        return;

    const i32 start_x = u_clamp(x,      0, (i32)ctx->curr_buf->w - 1);
    const i32 end_x   = u_clamp(x + w,  0, (i32)ctx->curr_buf->w - 1);
    const i32 start_y = u_clamp(y,      0, (i32)ctx->curr_buf->h - 1);
    const i32 end_y   = u_clamp(y + h,  0, (i32)ctx->curr_buf->h - 1);

    if (start_x == end_x || start_y == end_y)
        return;

    register pixel_t *const buf = ctx->curr_buf->buf;
    register const u32 stride = ctx->curr_buf->w;
    register const color_RGBA32_t color = ctx->current_color;

    for (register i32 y_ = start_y; y_ < end_y; y_++) {
        for (register i32 x_ = start_x; x_ < end_x; x_++) {
            r_putpixel_fast_matching_pixelfmt_(buf, x_, y_, stride, color);
        }
    }
}
