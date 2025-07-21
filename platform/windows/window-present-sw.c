#define P_INTERNAL_GUARD__
#include "window-present-sw.h"
#undef P_INTERNAL_GUARD__
#define P_INTERNAL_GUARD__
#include "error.h"
#undef P_INTERNAL_GUARD__
#include "../event.h"
#include "../thread.h"
#include "../window.h"
#include <core/log.h>
#include <core/util.h>
#include <stdbool.h>
#include <stdatomic.h>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif /* WIN32_LEAN_AND_MEAN */
#include <windows.h>
#include <wingdi.h>
#include <windef.h>

#define MODULE_NAME "window-present-sw"

i32 render_init_software(struct window_render_software_ctx *ctx,
    const struct p_window_info *win_info, HWND win_handle)
{
    memset(ctx, 0, sizeof(struct window_render_software_ctx));
    ctx->initialized_ = true;

    const u32 w = win_info->client_area.w;
    const u32 h = win_info->client_area.h;

    /* Initialize the pixel buffers */
    ctx->buffers_[0].w = ctx->buffers_[1].w = w;
    ctx->buffers_[0].h = ctx->buffers_[1].h = h;

    ctx->buffers_[0].buf = calloc(w * h, sizeof(pixel_t));
    if (ctx->buffers_[0].buf == NULL)
        goto_error("Failed to allocate the first pixel buffer");

    ctx->buffers_[1].buf = calloc(w * h, sizeof(pixel_t));
    if (ctx->buffers_[1].buf == NULL)
        goto_error("Failed to allocate the second pixel buffer");

    ctx->back_fb = &ctx->buffers_[0];
    ctx->front_fb = &ctx->buffers_[0];

    /* Get the device context of the window */
    ctx->dc = GetDC(win_handle);
    if (ctx->dc == NULL) {
        goto_error("Failed to get the device context of the window: %s",
            get_last_error_msg());
    }

    ctx->win_handle = win_handle;

    atomic_flag_clear(&ctx->swap_busy);
    ctx->swap_req_mutex = p_mt_mutex_create();

    return 0;

err:
    render_destroy_software(ctx);
    return 1;
}

struct pixel_flat_data * render_present_software(
    struct window_render_software_ctx *ctx)
{
    s_assert(atomic_flag_test_and_set(&ctx->swap_busy),
        "The busy flag must be set during a swap!");

    /* Swap the buffers */
    struct pixel_flat_data *const tmp = ctx->back_fb;
    ctx->back_fb = ctx->front_fb;
    ctx->front_fb = tmp;

    const BITMAPINFO bmi = {
        .bmiHeader = {
            .biSize = sizeof(BITMAPINFO),
            .biWidth = ctx->front_fb->w,
            .biHeight = -ctx->front_fb->h,
            .biPlanes = 1,
            .biBitCount = 32,
            .biCompression = BI_RGB
        },
    };
#define DST_X 0
#define DST_Y 0
#define SRC_X 0
#define SRC_Y 0
#define WIDTH ctx->front_fb->w
#define HEIGHT ctx->front_fb->h
#define START_SCANLINE 0
#define N_LINES HEIGHT

    i32 ret = SetDIBitsToDevice(ctx->dc, DST_X, DST_Y, WIDTH, HEIGHT,
        SRC_X, SRC_Y, START_SCANLINE, N_LINES,
        ctx->front_fb->buf, &bmi, DIB_RGB_COLORS);
    if (ret != (i32)HEIGHT) {
        s_log_error("SetDIBitsToDevice failed (ret %d - should be %d)",
            ret, HEIGHT);
        p_event_send(&(const struct p_event) {
            .type = P_EVENT_PAGE_FLIP,
            .info.page_flip_status = 1
        });
    } else {
        p_event_send(&(const struct p_event) {
            .type = P_EVENT_PAGE_FLIP,
            .info.page_flip_status = 0,
        });
    }

    return ctx->back_fb;
}

void render_destroy_software(struct window_render_software_ctx *ctx)
{
    if (!ctx->initialized_)
        return;

    if (ctx->swap_req_mutex != P_MT_MUTEX_NULL) {
        p_mt_mutex_lock(&ctx->swap_req_mutex);
        (void) atomic_flag_test_and_set(&ctx->swap_busy);
        p_mt_mutex_unlock(&ctx->swap_req_mutex);
        p_mt_mutex_destroy(&ctx->swap_req_mutex);
        atomic_flag_clear(&ctx->swap_busy);
    }

    ctx->initialized_ = false;

    if (ctx->dc != NULL) {
        s_assert(ctx->win_handle != NULL, "Invalid value of the window handle");
        if (ReleaseDC(ctx->win_handle, ctx->dc) == 0)
            s_log_error("The window's GDI device context was not released.");
        ctx->dc = NULL;
    }
    ctx->win_handle = NULL;

    if (ctx->buffers_[0].buf != NULL)
        u_nfree(&ctx->buffers_[0].buf);
    if (ctx->buffers_[1].buf != NULL)
        u_nfree(&ctx->buffers_[1].buf);
}
