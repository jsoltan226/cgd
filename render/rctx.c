#include "rctx.h"
#include "core/math.h"
#include <core/int.h>
#include <core/log.h>
#include <core/util.h>
#include <core/pixel.h>
#include <core/shapes.h>
#include <platform/window.h>
#include <platform/thread.h>
#include <stdlib.h>
#include <string.h>

#define R_INTERNAL_GUARD__
#include "rctx-internal.h"
#undef R_INTERNAL_GUARD__

#define MODULE_NAME "rctx"

static void swap_buffers(struct r_ctx *rctx);

struct r_ctx * r_ctx_init(struct p_window *win, enum r_type type, u32 flags)
{
    u_check_params(win != NULL);

    struct r_ctx *ctx = calloc(1, sizeof(struct r_ctx));
    s_assert(ctx != NULL, "calloc() for struct r_ctx failed!");

    switch (type) {
        case R_TYPE_OPENGL:
        case R_TYPE_VULKAN:
            s_log_warn("The Vulkan and OpenGL renderers are not yet implemented."
                " Falling back to software.");
        case R_TYPE_SOFTWARE:
        default: case R_TYPE_DEFAULT:
            ctx->type = R_TYPE_SOFTWARE;
            break;
    }

    ctx->win = win;
    if (p_window_get_meta(ctx->win, &ctx->win_meta))
        goto_error("Failed to get window metadata!");

    s_log_debug("win_meta->w: %u, win_meta->h: %u",
        ctx->win_meta.w, ctx->win_meta.h);

    ctx->buffers[0].buf = calloc(ctx->win_meta.w * ctx->win_meta.h, sizeof(pixel_t));
    ctx->buffers[1].buf = calloc(ctx->win_meta.w * ctx->win_meta.h, sizeof(pixel_t));
    s_assert(ctx->buffers[0].buf != NULL && ctx->buffers[1].buf != NULL,
        "calloc() failed for the pixel buffer(s)");

    ctx->buffers[0].w = ctx->buffers[1].w = ctx->win_meta.w;
    ctx->buffers[0].h = ctx->buffers[1].h = ctx->win_meta.h;
    u_rect_from_pixel_data(&ctx->buffers[0], &ctx->pixels_rect);

    ctx->render_buffer = &ctx->buffers[0];
    ctx->present_buffer = &ctx->buffers[1];
    ctx->current_color = BLACK_PIXEL;

    /* Prepare and start the thread */
    ctx->thread_info.mutex = p_mt_mutex_create();
    ctx->thread_info.running = p_mt_cond_create();

    if (p_mt_thread_create(&ctx->thread, renderer_main, &ctx->thread_info))
        goto_error("Failed to spawn the renderer thread!");

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

    p_mt_cond_signal(ctx->thread_info.running);
    p_mt_thread_wait(&ctx->thread);
    p_mt_cond_destroy(&ctx->thread_info.running);
    p_mt_mutex_destroy(&ctx->thread_info.mutex);
    free(ctx->buffers[0].buf);
    free(ctx->buffers[1].buf);
    u_nzfree(ctx_p);
}

void r_ctx_set_color(struct r_ctx *ctx, color_RGBA32_t color)
{
    u_check_params(ctx != NULL);

    if (ctx->win_meta.color_format == BGRA32 ||
        ctx->win_meta.color_format == BGRX32)
        u_rgba_swap_b_r(color);

    ctx->current_color = color;
}

void r_flush(struct r_ctx *ctx)
{
    u_check_params(ctx != NULL);
    swap_buffers(ctx);
    p_window_render(ctx->win, ctx->present_buffer);
}

void r_reset(struct r_ctx *ctx)
{
    u_check_params(ctx != NULL);
    memset(ctx->render_buffer->buf, 0,
        ctx->render_buffer->w * ctx->render_buffer->h * sizeof(pixel_t)
    );
}

static void swap_buffers(struct r_ctx *rctx)
{
    if (rctx->render_buffer == &rctx->buffers[0]) {
        rctx->present_buffer = &rctx->buffers[0];
        rctx->render_buffer = &rctx->buffers[1];
    } else {
        rctx->present_buffer = &rctx->buffers[1];
        rctx->render_buffer = &rctx->buffers[0];
    }
}
