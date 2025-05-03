#include "rctx.h"
#include "core/math.h"
#include <core/int.h>
#include <core/log.h>
#include <core/util.h>
#include <core/pixel.h>
#include <core/shapes.h>
#include <platform/window.h>
#include <platform/thread.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>

#define R_INTERNAL_GUARD__
#include "rctx-internal.h"
#undef R_INTERNAL_GUARD__

#define MODULE_NAME "rctx"

struct r_ctx * r_ctx_init(struct p_window *win, enum r_type type, u32 flags)
{
    u_check_params(win != NULL);
    (void) flags;

    struct r_ctx *ctx = calloc(1, sizeof(struct r_ctx));
    s_assert(ctx != NULL, "calloc() for struct r_ctx failed!");

    switch (type) {
        case R_TYPE_OPENGL:
        case R_TYPE_VULKAN:
            s_log_warn("The Vulkan and OpenGL renderers are not yet implemented."
                " Falling back to software.");
            /* Fall through */
        case R_TYPE_SOFTWARE:
        default: case R_TYPE_DEFAULT:
            ctx->type = R_TYPE_SOFTWARE;
            break;
    }

    ctx->win = win;
    p_window_get_info(ctx->win, &ctx->win_info);
    s_log_debug("win_rect->w: %u, win_rect->h: %u",
        ctx->win_info.client_area.w, ctx->win_info.client_area.h);

    if (ctx->win_info.gpu_acceleration != P_WINDOW_ACCELERATION_NONE) {
        if (p_window_set_acceleration(ctx->win, P_WINDOW_ACCELERATION_NONE))
            goto_error("Failed to set window acceleration");
    }

    ctx->curr_buf = p_window_swap_buffers(ctx->win,
        ctx->win_info.vsync_supported ?
            P_WINDOW_PRESENT_VSYNC :
            P_WINDOW_PRESENT_NOW
    );
    if (ctx->curr_buf == NULL)
        goto_error("Failed to swap buffers");
    u_rect_from_pixel_data(ctx->curr_buf, &ctx->pixels_rect);

    ctx->current_color = BLACK_PIXEL;

    /* Prepare and start the thread */
    ctx->thread_info.mutex = p_mt_mutex_create();
    ctx->thread_info.cond = p_mt_cond_create();
    atomic_store(&ctx->thread_info.running, true);
    if (p_mt_thread_create(&ctx->thread, renderer_main, &ctx->thread_info)) {
        atomic_store(&ctx->thread_info.running, false);
        goto_error("Failed to spawn the renderer thread!");
    }

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

    if (atomic_load(&ctx->thread_info.running)) {
        atomic_store(&ctx->thread_info.running, false);
        p_mt_cond_signal(ctx->thread_info.cond);
        p_mt_thread_wait(&ctx->thread);
        p_mt_cond_destroy(&ctx->thread_info.cond);
        p_mt_mutex_destroy(&ctx->thread_info.mutex);
    }
    u_nzfree(ctx_p);
}

void r_ctx_set_color(struct r_ctx *ctx, color_RGBA32_t color)
{
    u_check_params(ctx != NULL);

    if (ctx->win_info.display_color_format == BGRA32 ||
        ctx->win_info.display_color_format == BGRX32)
        u_rgba_swap_b_r(color);

    ctx->current_color = color;
}

void r_flush(struct r_ctx *ctx)
{
    u_check_params(ctx != NULL);
    ctx->curr_buf = p_window_swap_buffers(ctx->win,
        ctx->win_info.vsync_supported ?
            P_WINDOW_PRESENT_VSYNC :
            P_WINDOW_PRESENT_NOW
    );
    s_assert(ctx->curr_buf != NULL, "Failed to swap buffers!");
}

void r_reset(struct r_ctx *ctx)
{
    u_check_params(ctx != NULL);
    memset(ctx->curr_buf->buf, 0,
        ctx->curr_buf->w * ctx->curr_buf->h * sizeof(pixel_t)
    );
}
