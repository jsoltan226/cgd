#include "rctx.h"
#include <core/int.h>
#include <core/log.h>
#include <core/util.h>
#include <core/pixel.h>
#include <core/shapes.h>
#include <platform/window.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>

#define R_INTERNAL_GUARD__
#include "rctx-internal.h"
#undef R_INTERNAL_GUARD__

#define MODULE_NAME "rctx"

struct r_ctx * r_ctx_init(struct p_window *win, enum r_type type, u32 flags)
{
    u_check_params(win != NULL);

    struct r_ctx *ctx = malloc(sizeof(struct r_ctx));
    s_assert(ctx != NULL, "malloc() for struct r_ctx failed!");

    switch (type) {
        case R_TYPE_OPENGL:
        case R_TYPE_VULKAN:
            s_log_warn("The Vulkan and OpenGL renderers are not yet implemented."
                "Falling back to software.");
            break;
        case R_TYPE_SOFTWARE:
        default: case R_TYPE_DEFAULT:
            ctx->type = R_TYPE_SOFTWARE;
            break;
    }

    ctx->win = win;
    if (p_window_get_meta(ctx->win, &ctx->win_meta))
        goto_error("Failed to get window metadata!");

    s_log_debug("win_meta->w: %u, win_meta->h: %u", ctx->win_meta.w, ctx->win_meta.h);

    ctx->pixels.buf = calloc(ctx->win_meta.w * ctx->win_meta.h, sizeof(pixel_t));
    s_assert(ctx->pixels.buf != NULL, "calloc() failed for the pixel buffer");

    ctx->pixels.w = ctx->win_meta.w;
    ctx->pixels.h = ctx->win_meta.h;

    p_window_bind_fb(win, &ctx->pixels);

    ctx->current_color = BLACK_PIXEL;

    /* Render 1 empty frame on init
     * to avoid junk uninitialized data being displayed */
    r_reset(ctx);
    r_flush(ctx);

    return ctx;

err:
    r_ctx_destroy(&ctx);
    return NULL;
}

void r_ctx_destroy(struct r_ctx **ctx_p)
{
    if (ctx_p == NULL || *ctx_p == NULL) return;
    struct r_ctx *ctx = *ctx_p;

    p_window_unbind_fb(ctx->win);
    u_nzfree(ctx);
}

void r_ctx_set_color(struct r_ctx *ctx, color_RGBA32_t color)
{
    u_check_params(ctx != NULL);

    if (ctx->win_meta.color_type == P_WINDOW_BGRA8888)
        u_rgba_swap_b_r(color);

    ctx->current_color = color;
}

void r_flush(struct r_ctx *ctx)
{
    u_check_params(ctx != NULL);
    p_window_render(ctx->win);
}

void r_reset(struct r_ctx *ctx)
{
    u_check_params(ctx != NULL);
    memset(ctx->pixels.buf, 0, ctx->pixels.w * ctx->pixels.h * sizeof(pixel_t));
}
