#include "putpixel.h"
#include <platform/window.h>
#include <stdlib.h>
#define R_INTERNAL_GUARD__
#include "rctx-internal.h"
#undef R_INTERNAL_GUARD__
#define R_INTERNAL_GUARD__
#include "putpixel-fast.h"
#undef R_INTERNAL_GUARD__

void r_putpixel(struct r_ctx *ctx, i32 x, i32 y, pixel_t val)
{
    if (ctx == NULL || x >= ctx->pixels.w || y >= ctx->pixels.h) return;

    r_putpixel_fast_(ctx->pixels.buf, x, y, ctx->pixels.w, val,
        ctx->win_meta.color_type);
}
