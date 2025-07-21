#include <string.h>
#define WINDOW_THREAD__
#define P_INTERNAL_GUARD__
#include "window-present-sw.h"
#undef P_INTERNAL_GUARD__
#undef WINDOW_THREAD__
#define P_INTERNAL_GUARD__
#include "error.h"
#undef P_INTERNAL_GUARD__
#define P_INTERNAL_GUARD__
#include "window-thread.h"
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

static i32 do_render_init_software(struct window_render_software_ctx *ctx,
    const struct p_window_info *win_info);
static i32 do_render_present_software(struct window_render_software_ctx *ctx,
    HWND target_window);
static void do_render_destroy_software(struct window_render_software_ctx *ctx);

static i32 create_rgb_bitmap_buffer(u32 w, u32 h,
    struct window_render_software_buf *o_buf);
static void destroy_rgb_bitmap_buffer(struct window_render_software_buf *buf);

i32 render_software_request_init_and_wait(HWND win_handle,
    struct window_render_software_ctx *ctx, const struct p_window_info *win_info
)
{
    struct render_init_software_req req = {
        .ctx = ctx,
        .win_info = win_info,
    };

    return
        window_thread_request_operation_and_wait(win_handle,
            REQ_OP_RENDER_INIT_SOFTWARE,
            P_MT_MUTEX_NULL, &req);
}

struct pixel_flat_data * render_software_swap_and_request_present(
    HWND target_window, struct window_render_software_ctx *ctx
)
{
    u_check_params(target_window != NULL && ctx != NULL &&
        atomic_load(&ctx->initialized_));

    p_mt_mutex_lock(&ctx->swap_mutex);
    {
        if (ctx->swap_done) {
            p_mt_mutex_unlock(&ctx->swap_mutex);
            return NULL; /* Another page flip in progress; drop frame */
        }

        /* Swap the buffers */
        struct window_render_software_buf *const tmp = ctx->front_buf;
        ctx->front_buf = ctx->back_buf;
        ctx->back_buf = tmp;

        ctx->user_ret.w = ctx->back_buf->w;
        ctx->user_ret.h = ctx->back_buf->h;
        ctx->user_ret.buf = ctx->back_buf->pixels;

        ctx->swap_done = true;

        /* These structs are only written to here
         * and they're protected by the mutex and `swap_done` flag,
         * so we don't have to worry about their previous values */
        ctx->front_buf->present_req_arg.target_window = target_window;
        ctx->front_buf->present_req_arg.ctx = ctx;

        struct window_thread_request *const req = &ctx->front_buf->present_req;
        req->type = REQ_OP_RENDER_PRESENT_SOFTWARE;
        req->arg = &ctx->front_buf->present_req_arg;
        req->win_handle = target_window;
        req->status_mutex = ctx->swap_mutex;
        req->status = REQ_STATUS_NOT_STARTED;
        req->completion_cond = P_MT_COND_NULL;

        window_thread_request_operation(req);
    }
    p_mt_mutex_unlock(&ctx->swap_mutex);

    return &ctx->user_ret;
}

void render_software_request_destruction_and_wait(HWND win_handle,
    struct window_render_software_ctx *ctx)
{
    struct render_destroy_software_req req = { .ctx = ctx };
    (void) window_thread_request_operation_and_wait(
        win_handle, REQ_OP_RENDER_DESTROY_SOFTWARE,
        P_MT_MUTEX_NULL, &req
    );
}

enum window_thread_request_status render_software_handle_window_thread_request(
    enum window_thread_request_type req_type, void *arg
)
{
    const union {
        struct render_init_software_req *init;
        struct render_present_software_req *present;
        struct render_destroy_software_req *destroy;
        void *voidp;
    } a = { .voidp = arg };

    enum window_thread_request_status ret = REQ_STATUS_PENDING;

    switch (req_type) {
    case REQ_OP_RENDER_INIT_SOFTWARE:
        ret = do_render_init_software(a.init->ctx, a.init->win_info) == 0
            ? REQ_STATUS_SUCCESS : REQ_STATUS_FAILURE;
        break;
    case REQ_OP_RENDER_PRESENT_SOFTWARE:
        ret = do_render_present_software(
                a.present->ctx, a.present->target_window
            ) == 0 ? REQ_STATUS_SUCCESS : REQ_STATUS_FAILURE;
        break;
    case REQ_OP_RENDER_DESTROY_SOFTWARE:
        do_render_destroy_software(a.destroy->ctx);
        ret = REQ_STATUS_SUCCESS;
        break;
    default:
        s_log_fatal("Invalid RENDER_*_SOFTWARE request type %d", req_type);
    }

    return ret;
}

static i32 do_render_init_software(struct window_render_software_ctx *ctx,
    const struct p_window_info *win_info)
{
    atomic_store(&ctx->initialized_, true);
    ctx->swap_mutex = p_mt_mutex_create();
    ctx->swap_done = false;

    /* Init the bitmap buffers */
    const u32 w = win_info->client_area.w, h = win_info->client_area.h;
    if (create_rgb_bitmap_buffer(w, h, &ctx->window_thread_data_.buffers[0]) ||
        create_rgb_bitmap_buffer(w, h, &ctx->window_thread_data_.buffers[1])
    ) {
        goto_error("Failed to create the pixel buffers");
    }

    /* Create a memory DC compatible with the current screen */
    ctx->window_thread_data_.memdc = CreateCompatibleDC(NULL);
    if (ctx->window_thread_data_.memdc == NULL)
        goto_error("Failed to create the in-memory DC");

    ctx->back_buf = &ctx->window_thread_data_.buffers[0];
    ctx->front_buf = &ctx->window_thread_data_.buffers[1];

    /* Select the new front buffer bitmap as the memdc's object,
     * keeping the old "default" bitmap to enable clean destruction of the DC */
    ctx->window_thread_data_.memdc_old_bitmap =
        SelectObject(ctx->window_thread_data_.memdc, ctx->front_buf->bmp);
    if (ctx->window_thread_data_.memdc_old_bitmap == NULL)
        goto_error("Failed to select the bitmap as the memory DC's object: %s",
            get_last_error_msg());

    ctx->user_ret.w = ctx->back_buf->w;
    ctx->user_ret.h = ctx->back_buf->h;
    ctx->user_ret.buf = ctx->back_buf->pixels;

    return 0;

err:
    do_render_destroy_software(ctx);
    return 1;
}

static i32 do_render_present_software(struct window_render_software_ctx *ctx,
    HWND target_window)
{
    u_check_params(target_window != NULL && atomic_load(&ctx->initialized_));

    i32 ret = 0;
    p_mt_mutex_lock(&ctx->swap_mutex);
    {
        s_assert(ctx->swap_done,
            "The buffers must be swapped before attempting presentation!");

        /* Get the window's device context */
        HDC win_dc = GetDC(target_window);
        if (win_dc == NULL) {
            ret = 1;
            s_log_error("Failed to get the window's device context: %s",
                get_last_error_msg());
            goto present_end;
        }

        /* Select the new front buffer into the memory DC */
        HBITMAP SO_ret = SelectObject(ctx->window_thread_data_.memdc,
            ctx->front_buf->bmp);
        if (SO_ret != ctx->back_buf->bmp)
        {
            s_log_error("Failed to `SelectObject` the new front buffer "
                "to the memory DC (ret %p): %s",
                SO_ret, get_last_error_msg());
            goto present_end;
        }

        /* Present the new front buffer (memdc -> win_dc) */
        ret = !BitBlt(win_dc, 0, 0,
            ctx->front_buf->w,
            ctx->front_buf->h,
            ctx->window_thread_data_.memdc,
            0, 0, SRCCOPY
        );
        if (ret) {
            s_log_error("BitBlt failed: %s", get_last_error_msg());
            goto present_end;
        }

present_end:
        if (win_dc != NULL) {
            if (ReleaseDC(target_window, win_dc) == 0)
                s_log_error("The temporary window DC was not released");
            win_dc = NULL;
        }

        ctx->swap_done = false;
    }
    p_mt_mutex_unlock(&ctx->swap_mutex);

    /* Swap the buffers */
    p_event_send(&(const struct p_event) {
        .type = P_EVENT_PAGE_FLIP,
        .info.page_flip_status = ret != 0,
    });

    return ret;
}

static void do_render_destroy_software(struct window_render_software_ctx *ctx)
{
    if (!atomic_exchange(&ctx->initialized_, false))
        return;

    if (ctx->swap_mutex != P_MT_MUTEX_NULL) {
        ctx->swap_done = true;
        p_mt_mutex_lock(&ctx->swap_mutex);
        p_mt_mutex_unlock(&ctx->swap_mutex);
        p_mt_mutex_destroy(&ctx->swap_mutex);
        ctx->swap_done = false;
    }

    ctx->user_ret.buf = NULL;
    ctx->user_ret.w = ctx->user_ret.h = 0;

    if (ctx->window_thread_data_.memdc != NULL) {
        if (ctx->window_thread_data_.memdc_old_bitmap != NULL) {
            HBITMAP old_bitmap = SelectObject(ctx->window_thread_data_.memdc,
                ctx->window_thread_data_.memdc_old_bitmap);
            if (!(old_bitmap == ctx->back_buf->bmp ||
                old_bitmap == ctx->front_buf->bmp)
            ) {
                s_log_error("Failed to select the old default bitmap as the "
                    "memory DC's new object before destruction (ret %p): %s",
                    old_bitmap, get_last_error_msg());
            }
            ctx->window_thread_data_.memdc_old_bitmap = NULL;
        }
        if (DeleteDC(ctx->window_thread_data_.memdc) == 0)
            s_log_error("The window's GDI device context was not released.");
        ctx->window_thread_data_.memdc = NULL;
    }

    ctx->front_buf = ctx->back_buf = NULL;
    destroy_rgb_bitmap_buffer(&ctx->window_thread_data_.buffers[0]);
    destroy_rgb_bitmap_buffer(&ctx->window_thread_data_.buffers[1]);
}

static i32 create_rgb_bitmap_buffer(u32 w, u32 h,
    struct window_render_software_buf *o_buf)
{
    o_buf->w = w;
    o_buf->h = h;
    memset(&o_buf->present_req, 0, sizeof(struct window_thread_request));
    memset(&o_buf->present_req_arg, 0,
        sizeof(struct render_present_software_req));

    /* Used only with indexed color palettes.
     * We are creating an RGB bitmap, so we leave this as `NULL` */
    const HDC DC = NULL;

    /* The actual bitmap parameters */
    const BITMAPINFO BMI = {
        .bmiHeader = {
            .biSize = sizeof(BITMAPINFOHEADER),
            .biWidth = w,
            .biHeight = h,
            .biPlanes = 1,
            .biBitCount = 32,
            .biCompression = BI_RGB, /* uncompressed RGB image */
            /* The rest of the fields are irrelevant here
             * and can be 0 */
        },
    };

    /* The type of data contained by the bitmap -
     * indexed color palette, or, in our case, plain RGB pixels */
    const UINT USAGE = DIB_RGB_COLORS;

    /* A pointer to the pointer to which `CreateDIBSection` will write
     * the memory address of the raw pixel bytes */
    void **const OUT_BITS_P = (void **)&o_buf->pixels;

    /* Used for creating a bitmap mapped to a file.
     * Irrelevant here */
    const HANDLE SECTION = NULL;

    /* Also used when creating a bitmap from a file mapping,
     * and also irrelevant */
    const DWORD OFFSET = 0;

    o_buf->bmp = CreateDIBSection(DC, &BMI, USAGE, OUT_BITS_P, SECTION, OFFSET);
    if (o_buf->bmp == NULL) {
        /* In case of failure, `CreateDIBSection` will also write `NULL`
         * to `*OUT_BITS_P`, so we don't have to do that here */
        o_buf->w = o_buf->h = 0;
        s_log_error("Failed to create buffer bitmap: %s", get_last_error_msg());
        return 1;
    }

    return 0;
}

static void destroy_rgb_bitmap_buffer(struct window_render_software_buf *buf)
{
    if (buf->bmp != NULL) {
        if (DeleteObject(buf->bmp) == 0) {
            s_log_error("Failed to delete the bitmap object: %s",
                get_last_error_msg());
        }
    }
    buf->bmp = NULL;
    buf->pixels = NULL;
    buf->w = buf->h = 0;
    memset(&buf->present_req, 0, sizeof(struct window_thread_request));
    memset(&buf->present_req_arg, 0,
        sizeof(struct render_present_software_req));
}
