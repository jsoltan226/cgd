#include "putpixel.h"
#include "platform/window.h"
#include <stdlib.h>
#define R_INTERNAL_GUARD__
#include "rctx_internal.h"
#undef R_INTERNAL_GUARD__

void r_putpixel(struct r_ctx *ctx, i32 x, i32 y, pixel_t val)
{
    if (ctx == NULL || x >= ctx->pixels.w || y >= ctx->pixels.h) return;

    if (ctx->win_meta.color_type == P_WINDOW_BGRA8888) {
        u8 tmp = val.r;
        val.r = val.b;
        val.b = tmp;
    }

    ctx->pixels.buf[y * ctx->pixels.w + x] = val;
}
