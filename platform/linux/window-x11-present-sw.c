#define _GNU_SOURCE
#define P_INTERNAL_GUARD__
#define X11_RENDER_SOFTWARE_FB_TYPE_LIST_DEF__
#include "window-x11-present-sw.h"
#undef X11_RENDER_SOFTWARE_FB_TYPE_LIST_DEF__
#undef P_INTERNAL_GUARD__
#define P_INTERNAL_GUARD__
#include "libxcb-rtld.h"
#undef P_INTERNAL_GUARD__
#define P_INTERNAL_GUARD__
#include "window-x11-extensions.h"
#undef P_INTERNAL_GUARD__
#include "../event.h"
#include "../window.h"
#include <core/int.h>
#include <core/log.h>
#include <core/util.h>
#include <core/spinlock.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdatomic.h>
#include <signal.h>
#include <pthread.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <xcb/xcb.h>
#include <xcb/xproto.h>

#define MODULE_NAME "window-x11-present-sw"

static i32 software_init_buffer_malloced(
    struct x11_render_software_malloced_image_buf *buffer_o,
    struct pixel_flat_data *pixbuf_o,
    const struct x11_render_software_generic_window_info *win_info
);
static i32 software_init_buffer_shm(
    struct x11_render_software_shm_buf *buffer_o,
    struct pixel_flat_data *pixbuf_o,
    const struct x11_render_software_generic_window_info *win_info,
    xcb_connection_t *conn, const struct libxcb *xcb
);
static i32 software_init_buffer_pixmap(
    struct x11_render_software_present_pixmap_buf *buffer_o,
    struct pixel_flat_data *pixbuf_o,
    const struct x11_render_software_generic_window_info *win_info,
    xcb_connection_t *conn, const struct libxcb *xcb
);

static i32 software_present_malloced(
    const struct x11_render_software_malloced_image_buf *buf,
    struct x11_render_shared_malloced_data *shared_data
);
static i32 software_present_shm(
    const struct x11_render_software_shm_buf *buf,
    struct x11_render_shared_shm_data *shared_data,
    const struct x11_render_software_generic_window_info *win_info,
    xcb_connection_t *conn, const struct libxcb *xcb
);
static i32 software_present_pixmap(
    const struct x11_render_software_present_pixmap_buf *buf,
    struct x11_render_shared_present_data *shared_data,
    enum p_window_present_mode present_mode,
    const struct x11_render_software_generic_window_info *win_info,
    xcb_connection_t *conn, const struct libxcb *xcb
);
static void software_destroy_buffer_malloced(
    struct x11_render_software_malloced_image_buf *buf
);
static void software_destroy_buffer_shm(
    struct x11_render_software_shm_buf *buf,
    xcb_connection_t *conn, const struct libxcb *xcb
);
static void software_destroy_buffer_pixmap(
    struct x11_render_software_present_pixmap_buf *buf,
    xcb_connection_t *conn, const struct libxcb *xcb
);

static i32 do_init_buffer(struct x11_render_software_buf *buf,
    struct x11_render_shared_buffer_data *shared_buf_data,
    struct x11_render_software_ctx *ro_sw_rctx_handle__,
    const struct x11_render_software_generic_window_info *win_info,
    xcb_connection_t *conn, const struct libxcb *xcb);

static i32 init_malloced_shared_data(
    struct x11_render_shared_malloced_data *shared_data,
    struct x11_render_software_ctx *ro_sw_rctx_handle,
    const struct x11_render_software_generic_window_info *win_info,
    xcb_connection_t *conn, const struct libxcb *xcb
);
static void destroy_malloced_shared_data(
    struct x11_render_shared_malloced_data *shared_data
);

static i32 init_shm_shared_data(struct x11_render_shared_shm_data *shared_data);
static void destroy_shm_shared_data(
    struct x11_render_shared_shm_data *shared_data
);

static i32 init_present_shared_data(
    struct x11_render_shared_present_data *shared_data,
    xcb_window_t win_handle, xcb_connection_t *conn, const struct libxcb *xcb
);
static void destroy_present_shared_data(
    struct x11_render_shared_present_data *shared_data,
    xcb_window_t win_handle, xcb_connection_t *conn, const struct libxcb *xcb
);

static i32 attach_shm(xcb_shm_segment_info_t *shm_o, u32 w, u32 h,
    const struct x11_extension_store *ext_store,
    xcb_connection_t *conn, const struct libxcb *xcb);
static void detach_shm(xcb_shm_segment_info_t *shm,
    xcb_connection_t *conn, const struct libxcb *xcb);

static void * malloced_present_thread_fn(void *arg);

i32 X11_render_init_software(struct x11_render_software_ctx *sw_rctx,
    u16 win_w, u16 win_h, u8 root_depth, u64 max_request_size,
    xcb_window_t win_handle, const struct x11_extension_store *ext_store,
    xcb_connection_t *conn, const struct libxcb *xcb,
    bool *o_vsync_supported)
{
    *o_vsync_supported = false;
    sw_rctx->initialized_ = true;
    atomic_flag_clear(&sw_rctx->present_pending);
    spinlock_init(&sw_rctx->swap_lock);

    /* Create the window's graphics context (GC) */
    sw_rctx->window_gc = xcb->xcb_generate_id(conn);

#define GCV_MASK XCB_GC_FOREGROUND
    const u32 gcv[] = {
        0 /* black pixel */
    };

    xcb_generic_error_t *e = NULL;
    xcb_void_cookie_t vc = xcb->xcb_create_gc_checked(conn,
        sw_rctx->window_gc, win_handle, GCV_MASK, gcv);
    if (e = xcb->xcb_request_check(conn, vc), e != NULL) {
        u_nzfree(&e);
        s_log_error("Failed to create the graphics context!");
        sw_rctx->window_gc = XCB_NONE;
        return 1;
    }

    sw_rctx->generic_win_info.max_request_size = max_request_size;
    sw_rctx->generic_win_info.win_w = win_w;
    sw_rctx->generic_win_info.win_h = win_h;
    sw_rctx->generic_win_info.root_depth = root_depth;
    sw_rctx->generic_win_info.win_handle = win_handle;
    sw_rctx->generic_win_info.win_gc = sw_rctx->window_gc;
    sw_rctx->generic_win_info.ext_store = ext_store;

    atomic_store(&sw_rctx->shared_buf_data.malloced.initialized_, false);
    atomic_store(&sw_rctx->shared_buf_data.shm.initialized_, false);
    atomic_store(&sw_rctx->shared_buf_data.present.initialized_, false);

    for (u32 i = 0; i < 2; i++) {
        if (do_init_buffer(&sw_rctx->buffers[i], &sw_rctx->shared_buf_data,
            sw_rctx, &sw_rctx->generic_win_info, conn, xcb))
        {
            s_log_error("Couldn't initialize buffer %u in any way", i);
        } else {
            s_assert(sw_rctx->buffers[i].type >= 0 &&
                sw_rctx->buffers[i].type < X11_SWFB_N_TYPES_,
                "Invalid software buffer type %d", sw_rctx->buffers[i].type);
#ifndef CGD_BUILDTYPE_RELEASE
#define X_(name) #name,
            static const char *const swfb_type_strings[X11_SWFB_N_TYPES_] = {
                X11_RENDER_SOFTWARE_FB_TYPE_LIST
            };
#undef X_
            s_log_debug("Successfully initialized software rendering buffer %i"
                " as %s", i, swfb_type_strings[sw_rctx->buffers[i].type]);
#endif /* CGD_BUILDTYPE_RELEASE */
        }
    }

    /* vsync is only supported by PRESENT_PIXMAP buffers */
    *o_vsync_supported =
        sw_rctx->buffers[0].type == X11_SWFB_PRESENT_PIXMAP &&
        sw_rctx->buffers[1].type == X11_SWFB_PRESENT_PIXMAP;

    sw_rctx->curr_front_buf = &sw_rctx->buffers[0];
    sw_rctx->curr_back_buf = &sw_rctx->buffers[1];

    return 0;
}

struct pixel_flat_data * X11_render_present_software(
    struct x11_render_software_ctx *sw_rctx,
    xcb_connection_t *conn, const struct libxcb *xcb,
    enum p_window_present_mode present_mode
)
{
    sw_rctx->total_frames++;
    /* We assume that `present_mode` has already been validated */

    if (atomic_flag_test_and_set(&sw_rctx->present_pending)) {
        sw_rctx->dropped_frames++;
        return &sw_rctx->curr_back_buf->pixbuf;
    }
    /* Reset after the page flip completes */

    i32 swap_ret = 0;
    struct x11_render_software_buf *const curr_buf = sw_rctx->curr_back_buf;


    switch (curr_buf->type) {
    case X11_SWFB_NULL:
        s_log_fatal("Attempt to present an uninitialized buffer");
    case X11_SWFB_MALLOCED_IMAGE:
        swap_ret = software_present_malloced(&curr_buf->fb.malloced,
            &sw_rctx->shared_buf_data.malloced);
        /* `X11_render_software_finish_frame` get called
         * by the special present thread when it completes a presentation */
        break;
    case X11_SWFB_SHMSEG:
        swap_ret = software_present_shm(&curr_buf->fb.shm,
            &sw_rctx->shared_buf_data.shm,
            &sw_rctx->generic_win_info, conn, xcb);
        /* `X11_render_software_finish_frame` gets called
         * by the listener thread when it receives a SHM_COMPLETION event */
        break;
    case X11_SWFB_PRESENT_PIXMAP:
        swap_ret = software_present_pixmap(&curr_buf->fb.present_pixmap,
            &sw_rctx->shared_buf_data.present, present_mode,
            &sw_rctx->generic_win_info, conn, xcb);
        /* `X11_render_software_finish_frame` gets called
         * by the listener thread when it receives a PRESENT_COMPLETE event */
        break;
    default:
        s_log_fatal("Invalid buffer type %d", curr_buf->type);
    }

    if (xcb->xcb_flush(conn) <= 0)
        s_log_error("xcb_flush failed!");

    if (swap_ret == 0) {
        /* Swap the buffers only if the page flip succeeded */
        spinlock_acquire(&sw_rctx->swap_lock);
        {
            struct x11_render_software_buf *const tmp = sw_rctx->curr_front_buf;
            sw_rctx->curr_front_buf = sw_rctx->curr_back_buf;
            sw_rctx->curr_back_buf = tmp;
        }
        spinlock_release(&sw_rctx->swap_lock);
        s_log_trace("Page flip OK; swap buffers (new front: %p, new back: %p)",
            sw_rctx->curr_front_buf, sw_rctx->curr_back_buf);
    } else {
        atomic_flag_clear(&sw_rctx->present_pending);
        sw_rctx->dropped_frames++;
    }

    return &sw_rctx->curr_back_buf->pixbuf;
}

void X11_render_destroy_software(struct x11_render_software_ctx *sw_rctx,
    xcb_connection_t *conn, xcb_window_t win_handle, const struct libxcb *xcb)
{
    if (sw_rctx == NULL || !sw_rctx->initialized_)
        return;

    if (atomic_flag_test_and_set(&sw_rctx->present_pending))
        s_log_warn("Forcing buffer destruction while it might be in use");
    atomic_flag_clear(&sw_rctx->present_pending);

    if (atomic_load(&sw_rctx->shared_buf_data.present.initialized_)) {
        destroy_present_shared_data(&sw_rctx->shared_buf_data.present,
            win_handle, conn, xcb);
    }
    if (atomic_load(&sw_rctx->shared_buf_data.shm.initialized_)) {
        destroy_shm_shared_data(&sw_rctx->shared_buf_data.shm);
    }
    if (atomic_load(&sw_rctx->shared_buf_data.malloced.initialized_)) {
        destroy_malloced_shared_data(&sw_rctx->shared_buf_data.malloced);
    }

    for (u32 i = 0; i < 2; i++) {
        struct x11_render_software_buf *const curr_buf = &sw_rctx->buffers[i];
        switch (curr_buf->type) {
        case X11_SWFB_NULL: break;
        case X11_SWFB_MALLOCED_IMAGE:
            software_destroy_buffer_malloced(&curr_buf->fb.malloced);
            break;
        case X11_SWFB_SHMSEG:
            software_destroy_buffer_shm(&curr_buf->fb.shm, conn, xcb);
            break;
        case X11_SWFB_PRESENT_PIXMAP:
            software_destroy_buffer_pixmap(&curr_buf->fb.present_pixmap,
                conn, xcb);
            break;
        default:
            s_log_fatal("Invalid buffer type %d", curr_buf->type);
        }

        curr_buf->type = X11_SWFB_NULL;
        memset(&curr_buf->pixbuf, 0, sizeof(struct pixel_flat_data));
        curr_buf->initialized_ = false;
    }

    if (sw_rctx->window_gc != XCB_NONE) {
        xcb->xcb_free_gc(conn, sw_rctx->window_gc);
        sw_rctx->window_gc = XCB_NONE;
    }

    if (sw_rctx->total_frames != 0) {
        s_log_verbose("Total dropped frames: %lu/%lu (%lf%%)",
            sw_rctx->dropped_frames, sw_rctx->total_frames,
            ((f64)sw_rctx->dropped_frames / (f64)sw_rctx->total_frames) * 100.0
        );
    }

    sw_rctx->curr_back_buf = sw_rctx->curr_front_buf = NULL;
    sw_rctx->initialized_ = false;
}

void X11_render_software_finish_frame(struct x11_render_software_ctx *sw_rctx,
    bool status)
{
    atomic_flag_clear(&sw_rctx->present_pending);
    p_event_send(&(const struct p_event) {
        .type = P_EVENT_PAGE_FLIP,
        .info.page_flip_status = status
    });
}

static i32 software_init_buffer_malloced(
    struct x11_render_software_malloced_image_buf *buffer_o,
    struct pixel_flat_data *pixbuf_o,
    const struct x11_render_software_generic_window_info *win_info
)
{
    memset(buffer_o, 0, sizeof(struct x11_render_software_malloced_image_buf));
    memset(pixbuf_o, 0, sizeof(struct pixel_flat_data));

    buffer_o->w = win_info->win_w;
    buffer_o->h = win_info->win_h;
    buffer_o->buf = calloc(win_info->win_w * win_info->win_h, sizeof(pixel_t));
    if (buffer_o->buf == NULL) {
        s_log_error("Failed to calloc() the pixel buffer");
        return 1;
    }

    /* Fill in the pixel data struct */
    pixbuf_o->w = win_info->win_w;
    pixbuf_o->h = win_info->win_h;
    pixbuf_o->buf = buffer_o->buf;

    return 0;
}

static i32 software_init_buffer_shm(
    struct x11_render_software_shm_buf *buffer_o,
    struct pixel_flat_data *pixbuf_o,
    const struct x11_render_software_generic_window_info *win_info,
    xcb_connection_t *conn, const struct libxcb *xcb
)
{
    memset(buffer_o, 0, sizeof(struct x11_render_software_shm_buf));
    memset(pixbuf_o, 0, sizeof(struct pixel_flat_data));

    if (!X11_extension_is_available(win_info->ext_store, X11_EXT_SHM)) {
        s_log_error("The MIT-SHM extension is not available!");
        return -1;
    }

    buffer_o->root_depth = win_info->root_depth;
    buffer_o->w = win_info->win_w;
    buffer_o->h = win_info->win_h;

    /* Initialize the shm segment */
    if (attach_shm(&buffer_o->shm_info, win_info->win_w, win_info->win_h,
            win_info->ext_store, conn, xcb))
    {
        s_log_error("Failed to initialize the shared memory segment");
        return 1;
    }

    /* Fill in the pixel data struct */
    pixbuf_o->w = win_info->win_w;
    pixbuf_o->h = win_info->win_h;
    pixbuf_o->buf = (pixel_t *)buffer_o->shm_info.shmaddr;

    return 0;
}

static i32 software_init_buffer_pixmap(
    struct x11_render_software_present_pixmap_buf *buffer_o,
    struct pixel_flat_data *pixbuf_o,
    const struct x11_render_software_generic_window_info *win_info,
    xcb_connection_t *conn, const struct libxcb *xcb
)
{
    memset(buffer_o, 0, sizeof(struct x11_render_software_present_pixmap_buf));
    memset(pixbuf_o, 0, sizeof(struct pixel_flat_data));

    if (!X11_extension_is_available(win_info->ext_store, X11_EXT_SHM)) {
        s_log_error("The MIT-SHM extension is not available!");
        return -1;
    }

    if (!X11_extension_is_available(win_info->ext_store, X11_EXT_PRESENT)) {
        s_log_error("The X Present extension is not available!");
        return -1;
    }

    if (attach_shm(&buffer_o->shm_info, win_info->win_w, win_info->win_h,
            win_info->ext_store, conn, xcb))
    {
        s_log_error("Failed to initialize the shared memory segment");
        return 1;
    }

    buffer_o->w = win_info->win_w;
    buffer_o->h = win_info->win_h;
    buffer_o->root_depth = win_info->root_depth;

    buffer_o->pixmap = xcb->xcb_generate_id(conn);

    /* This request never fails */
    (void) xcb->shm.xcb_shm_create_pixmap(
        conn, buffer_o->pixmap, win_info->win_handle,
        win_info->win_w, win_info->win_h, win_info->root_depth,
        buffer_o->shm_info.shmseg, 0
    );

    pixbuf_o->w = win_info->win_w;
    pixbuf_o->h = win_info->win_h;
    pixbuf_o->buf = (pixel_t *)buffer_o->shm_info.shmaddr;

    return 0;
}

static i32 software_present_malloced(
    const struct x11_render_software_malloced_image_buf *buf,
    struct x11_render_shared_malloced_data *shared_data
)
{
    if (!atomic_load(&shared_data->present_thread_ready)) {
        s_log_warn("Present thread is not yet ready; dropping frame");
        return 1;
    }

    if (atomic_load(&shared_data->present_request_buffer) != NULL) {
        (void) pthread_cond_signal(&shared_data->present_request_cond);
        s_log_warn("Present thread is busy; dropping frame");
        return 1;
    }

    atomic_store(&shared_data->present_request_buffer, buf);
    s_log_trace("present_pending -> true");
    (void) pthread_cond_signal(&shared_data->present_request_cond);

    return 0;
}

static i32 software_present_shm(
    const struct x11_render_software_shm_buf *buf,
    struct x11_render_shared_shm_data *shared_data,
    const struct x11_render_software_generic_window_info *win_info,
    xcb_connection_t *conn, const struct libxcb *xcb
)
{
    const xcb_drawable_t TARGET_DRAWABLE = win_info->win_handle;
    const xcb_gcontext_t TARGET_GC = win_info->win_gc;

    const u16 TOTAL_SRC_IMAGE_W = buf->w;
    const u16 TOTAL_SRC_IMAGE_H = buf->h;
    const u16 SRC_X = 0, SRC_Y = 0;
    const u16 SRC_W = buf->w;
    const u16 SRC_H = buf->h;

    const i16 DST_X = 0, DST_Y = 0;
    const u8 DST_DEPTH = buf->root_depth;
    const u8 DST_IMAGE_FORMAT = XCB_IMAGE_FORMAT_Z_PIXMAP;

    const u8 SEND_BLIT_COMPLETE_EVENT = true;
    const xcb_shm_seg_t SHMSEG = buf->shm_info.shmseg;
    const u32 SRC_START_OFFSET = 0;

    xcb_void_cookie_t cookie = xcb->shm.xcb_shm_put_image(
        conn, TARGET_DRAWABLE, TARGET_GC,
        TOTAL_SRC_IMAGE_W, TOTAL_SRC_IMAGE_H, SRC_X, SRC_Y, SRC_W, SRC_H,
        DST_X, DST_Y, DST_DEPTH, DST_IMAGE_FORMAT,
        SEND_BLIT_COMPLETE_EVENT, SHMSEG, SRC_START_OFFSET
    );
    atomic_store(&shared_data->blit_request_sequence_number, cookie.sequence);

    return 0;
}

static i32 software_present_pixmap(
    const struct x11_render_software_present_pixmap_buf *buf,
    struct x11_render_shared_present_data *shared_data,
    enum p_window_present_mode present_mode,
    const struct x11_render_software_generic_window_info *win_info,
    xcb_connection_t *conn, const struct libxcb *xcb
)
{
    /* The serial number that we use to track each page flip request.
     * Incremented on each request so that it's unique */
    const u32 SERIAL = atomic_load(&shared_data->serial) + 1;
    atomic_store(&shared_data->serial, SERIAL);

    /* The region of the pixmap that we mark as "containing updated pixels"
     * (the whole pixmap) */
    const xcb_xfixes_region_t VALID_PIXMAP_REGION = XCB_NONE;

    /* The region of the window that we mark as "ready for update"
     * (the whole window) */
    const xcb_xfixes_region_t WINDOW_UPDATE_REGION = XCB_NONE;

    /* The offset (from the top-left corner of the window)
     * at which the pixmap will be drawn */
    const i16 DST_OFFSET_X = 0, DST_OFFSET_Y = 0;

    /* Whichever CRTC the X server picks. We don't really care. */
    const xcb_randr_crtc_t TARGET_CRTC = XCB_NONE;

    /* The request will block until `WAIT_FENCE` is triggered,
     * but we don't want that */
    const xcb_sync_fence_t WAIT_FENCE = XCB_NONE;

    /* The fence that will be triggered when the blit is complete.
     * The client must guarantee that the pixmap won't be used
     * until that point, but we already get that information
     * in the event loop so this here is useless to us */
    const xcb_sync_fence_t IDLE_FENCE = XCB_NONE;

    /* Configuration options of the presentation.
     * We only care about whether we present at vsync or not. */
    const u32 OPTIONS = present_mode == P_WINDOW_PRESENT_NOW ?
        XCB_PRESENT_OPTION_ASYNC : XCB_PRESENT_OPTION_NONE;

    /* The presentation timing options.
     * We want it to happen as soon as possible. */
    const u64 TARGET_MONITOR_COUNTER = 0;
    const u64 DIVISOR = 0;
    const u64 REMAINDER = 0;

    /* We don't want to notify anything except our window
     * about the presentation completion event */
    const u32 NOTIFIES_LEN = 0;
    const xcb_present_notify_t *NOTIFIES = NULL;

    (void) xcb->present.xcb_present_pixmap(conn,
        win_info->win_handle, buf->pixmap,
        SERIAL, VALID_PIXMAP_REGION, WINDOW_UPDATE_REGION,
        DST_OFFSET_X, DST_OFFSET_Y, TARGET_CRTC, WAIT_FENCE, IDLE_FENCE,
        OPTIONS, TARGET_MONITOR_COUNTER, DIVISOR, REMAINDER,
        NOTIFIES_LEN, NOTIFIES);

    return 0;
}

static void software_destroy_buffer_malloced(
    struct x11_render_software_malloced_image_buf *buf
)
{
    if (buf->buf != NULL)
        u_nfree(&buf->buf);
    buf->w = buf->h = 0;
}

static void software_destroy_buffer_shm(
    struct x11_render_software_shm_buf *buf,
    xcb_connection_t *conn, const struct libxcb *xcb
)
{
    buf->root_depth = buf->w = buf->h = 0;

    detach_shm(&buf->shm_info, conn, xcb);
}

static void software_destroy_buffer_pixmap(
    struct x11_render_software_present_pixmap_buf *buf,
    xcb_connection_t *conn, const struct libxcb *xcb
)
{
    buf->root_depth = buf->w = buf->h = 0;

    if (buf->pixmap != XCB_NONE) {
        xcb_void_cookie_t vc =
            xcb->xcb_free_pixmap_checked(conn, buf->pixmap);
        xcb_generic_error_t *e = NULL;
        if (e = xcb->xcb_request_check(conn, vc), e != NULL) {
            s_log_error("xcb_free_pixmap failed (invalid pixmap handle)");
            u_nfree(&e);
        }
        buf->pixmap = XCB_NONE;
    }

    detach_shm(&buf->shm_info, conn, xcb);
}

static i32 do_init_buffer(struct x11_render_software_buf *buf,
    struct x11_render_shared_buffer_data *shared_buf_data,
    struct x11_render_software_ctx *ro_sw_rctx_handle__,
    const struct x11_render_software_generic_window_info *win_info,
    xcb_connection_t *conn, const struct libxcb *xcb)
{
    buf->initialized_ = true;
    buf->type = X11_SWFB_NULL;

    if (software_init_buffer_pixmap(&buf->fb.present_pixmap,
            &buf->pixbuf, win_info, conn, xcb) == 0)
    {
        /* Now that we have a pixmap buffer, to be able to use it
         * we need to init the struct containing the data that's shared
         * between all buffers (related to the present extension) */
        if (!atomic_load(&shared_buf_data->present.initialized_) &&
            init_present_shared_data(&shared_buf_data->present,
                win_info->win_handle, conn, xcb)
        ) {
            s_log_error("Failed to init SWFB_PRESENT_PIXMAP shared data");
            destroy_present_shared_data(&shared_buf_data->present,
                win_info->win_handle, conn, xcb);
            goto pixmap_fail;
        }
        buf->type = X11_SWFB_PRESENT_PIXMAP;
        return 0;
    } else {
pixmap_fail:
        s_log_warn("Failed to create window framebuffer with "
            "present/pixmap, falling back to shm");
        software_destroy_buffer_pixmap(&buf->fb.present_pixmap,
            conn, xcb);
    }

    if (software_init_buffer_shm(&buf->fb.shm,
            &buf->pixbuf, win_info, conn, xcb) == 0)
    {
        if (!atomic_load(&shared_buf_data->shm.initialized_) &&
            init_shm_shared_data(&shared_buf_data->shm)
        ) {
            s_log_error("Failed to init SWFB_SHMSEG shared data");
            destroy_shm_shared_data(&shared_buf_data->shm);
            goto shm_fail;
        }
        buf->type = X11_SWFB_SHMSEG;
        return 0;
    } else {
shm_fail:
        s_log_warn("Failed to create window framebuffer with shm, "
            "falling back to manually malloc'd buffer");
        software_destroy_buffer_shm(&buf->fb.shm, conn, xcb);
    }

    if (software_init_buffer_malloced(&buf->fb.malloced,
            &buf->pixbuf, win_info) == 0)
    {
        if (!atomic_load(&shared_buf_data->malloced.initialized_) &&
            init_malloced_shared_data(&shared_buf_data->malloced,
                ro_sw_rctx_handle__, win_info, conn, xcb)
        ) {
            s_log_error("Failed to init SWFB_MALLOCED_IMAGE shared data");
            destroy_malloced_shared_data(&shared_buf_data->malloced);
            goto malloced_fail;
        }
        buf->type = X11_SWFB_MALLOCED_IMAGE;
        return 0;
    } else {
malloced_fail:
        s_log_error("Failed to initialize manually allocated buffer");
        return 1;
    }

    s_log_fatal("impossible outcome");
}

static i32 init_malloced_shared_data(
    struct x11_render_shared_malloced_data *shared_data,
    struct x11_render_software_ctx *ro_sw_rctx_handle,
    const struct x11_render_software_generic_window_info *win_info,
    xcb_connection_t *conn, const struct libxcb *xcb
)
{
    if (atomic_load(&shared_data->initialized_)) {
        s_log_warn("SWFB_MALLOCED_IMAGE shared data already initialized; "
            "not doing anything.");
        return 0;
    }

    atomic_store(&shared_data->initialized_, true);

    shared_data->const_data.ro_sw_rctx_handle = ro_sw_rctx_handle;
    shared_data->const_data.conn = conn;

    (void) pthread_mutex_init(&shared_data->present_request_mutex, NULL);
    (void) pthread_cond_init(&shared_data->present_request_cond, NULL);
    atomic_store(&shared_data->present_request_buffer, NULL);
    atomic_store(&shared_data->present_thread_ready, false);
    (void) atomic_flag_test_and_set(&shared_data->present_thread_running);

    struct x11_render_software_malloced_present_thread_arg *thread_arg =
        malloc(sizeof(struct x11_render_software_malloced_present_thread_arg));
    s_assert(thread_arg != NULL, "malloc failed for present thread arg");

    thread_arg->sw_rctx_handle__ = ro_sw_rctx_handle;
    thread_arg->conn = conn;
    thread_arg->shared_data = shared_data;
    memcpy(&thread_arg->win_info, win_info,
        sizeof(struct x11_render_software_generic_window_info));
    thread_arg->xcb = xcb;

    i32 ret = pthread_create(&shared_data->present_thread, NULL,
        malloced_present_thread_fn, thread_arg);
    if (ret != 0) {
        atomic_flag_clear(&shared_data->present_thread_running);
        s_log_error("Failed to create the malloced buffer present thread: %s",
            strerror(ret));
        return 1;
    }

    /* The thread will free `thread_arg` when it's done reading from it */

    return 0;
}

static void destroy_malloced_shared_data(
    struct x11_render_shared_malloced_data *shared_data
)
{
    if (!atomic_load(&shared_data->initialized_))
        return;
    atomic_store(&shared_data->initialized_, false);

    atomic_flag_clear(&shared_data->present_thread_running);

    /* always successful */
    (void) pthread_cond_signal(&shared_data->present_request_cond);


    struct timespec thread_timeout;
    if (clock_gettime(CLOCK_REALTIME, &thread_timeout)) {
        s_log_error("Failed to get the current time: %s",
            strerror(errno));
        /* Give the thread 0.1s to clean itself up */
        usleep(100000);
        s_log_warn("Killing the listener thread...");
        pthread_kill(shared_data->present_thread, SIGKILL);

        goto thread_terminated;
    }

    /* Set the timeout to 1 second from now */
    thread_timeout.tv_sec += 1;

    i32 ret = pthread_timedjoin_np(shared_data->present_thread,
        NULL, &thread_timeout);
    if (ret == ETIMEDOUT) {
        s_log_error("Timed out while waiting for "
            "the event listener thread to terminate. "
            "Killing the thread...");
        pthread_kill(shared_data->present_thread, SIGKILL);
    } else if (ret != 0) {
        s_log_error("Failed to timed_join() the listener thread: %s. "
            "Killing the thread...", strerror(ret));
        pthread_kill(shared_data->present_thread, SIGKILL);
    } /* else OK */
thread_terminated:

    shared_data->present_thread = 0;
    atomic_store(&shared_data->present_thread_ready, false);

    /* Possible errors: EBUSY - The mutex is currently locked. P. */
    if (pthread_mutex_destroy(&shared_data->present_request_mutex) != 0) {
        s_log_error("Couldn't clean up the present request mutex "
            "because it's still locked");
    }

    /* Possible errors: EINVAL - The cond is not properly initialized.
     * Impossible. */
    s_assert(pthread_cond_destroy(&shared_data->present_request_cond) == 0,
        "impossible outcome");

    atomic_store(&shared_data->present_request_buffer, NULL);

    shared_data->const_data.ro_sw_rctx_handle = NULL;
    shared_data->const_data.conn = NULL;
}

static i32 init_shm_shared_data(struct x11_render_shared_shm_data *shared_data)
{
    if (atomic_load(&shared_data->initialized_)) {
        s_log_warn("SWFB_SHMSEG shared data already initialized; "
            "not doing anything.");
        return 0;
    }

    atomic_store(&shared_data->initialized_, true);
    atomic_store(&shared_data->blit_request_sequence_number, 0);

    return 0;
}

static void destroy_shm_shared_data(
    struct x11_render_shared_shm_data *shared_data)
{
    if (!atomic_load(&shared_data->initialized_))
        return;

    atomic_store(&shared_data->blit_request_sequence_number, 0);
    atomic_store(&shared_data->initialized_, false);
}

static i32 init_present_shared_data(
    struct x11_render_shared_present_data *shared_data,
    xcb_window_t win_handle, xcb_connection_t *conn, const struct libxcb *xcb
)
{
    if (atomic_load(&shared_data->initialized_)) {
        s_log_warn("SWFB_PRESENT_PIXMAP shared data already initialized; "
            "not doing anything.");
        return 0;
    }
    atomic_store(&shared_data->initialized_, true);
    atomic_store(&shared_data->serial, 0);

    i32 ret = 0;

    atomic_store(&shared_data->event_context_id, XCB_NONE);
    xcb_present_event_t tmp_event_context_id = xcb->xcb_generate_id(conn);
    xcb_void_cookie_t vc = xcb->present.xcb_present_select_input_checked(
        conn, tmp_event_context_id, win_handle,
        XCB_PRESENT_EVENT_MASK_COMPLETE_NOTIFY
    );
    xcb_generic_error_t *e = NULL;
    if (e = xcb->xcb_request_check(conn, vc), e != NULL) {
        s_log_error("xcb_present_select_input failed");
        u_nfree(&e);
        atomic_store(&shared_data->event_context_id, XCB_NONE);
        ret = 1;
    } else {
        atomic_store(&shared_data->event_context_id, tmp_event_context_id);
    }

    return ret;
}

static void destroy_present_shared_data(
    struct x11_render_shared_present_data *shared_data,
    xcb_window_t win_handle, xcb_connection_t *conn, const struct libxcb *xcb
)
{
    if (!atomic_load(&shared_data->initialized_))
        return;
    atomic_store(&shared_data->initialized_, false);

    const xcb_present_event_t event_context_id_value =
        atomic_load(&shared_data->event_context_id);
    if (event_context_id_value != XCB_NONE) {
        s_assert(&xcb->present._voidp_xcb_present_select_input_checked != NULL,
            "This function should be loaded at this point!");
        atomic_store(&shared_data->event_context_id, XCB_NONE);

        /* Delete the event context */
        xcb_void_cookie_t vc =
            xcb->present.xcb_present_select_input_checked(
                conn, shared_data->event_context_id, win_handle,
                0
            );
        xcb_generic_error_t *e = NULL;
        if (e = xcb->xcb_request_check(conn, vc), e != NULL) {
            s_log_error("xcb_present_select_input_checked failed");
            u_nfree(&e);
        }
    }

    atomic_store(&shared_data->serial, 0);
}

static i32 attach_shm(xcb_shm_segment_info_t *shm_o, u32 w, u32 h,
    const struct x11_extension_store *ext_store,
    xcb_connection_t *conn, const struct libxcb *xcb)
{
    if (!X11_extension_is_available(ext_store, X11_EXT_SHM)) {
        s_log_error("The MIT-SHM extension is not available!");
        return -1;
    }

    shm_o->shmaddr = (void *)-1;
    shm_o->shmseg = -1;

    /* Create the shmseg */
    shm_o->shmid = shmget(
        IPC_PRIVATE,
        w * h * sizeof(pixel_t),
        IPC_CREAT | 0600
    );
    if ((i32)shm_o->shmid == -1)
        goto_error("Failed to create shared memory: %s", strerror(errno));

    /* Attach (map) the segment to our address space */
    shm_o->shmaddr = shmat(shm_o->shmid, NULL, 0);
    if (shm_o->shmaddr == (void *)-1)
        goto_error("Failed to attach shared memory: %s", strerror(errno));

    shm_o->shmseg = xcb->xcb_generate_id(conn);

    /* Attach the segment to the X server */
    xcb_void_cookie_t vc = xcb->shm.xcb_shm_attach_checked( conn,
        shm_o->shmseg, shm_o->shmid, false);
    xcb_generic_error_t *e = NULL;
    if (e = xcb->xcb_request_check(conn, vc), e != NULL) {
        u_nzfree(&e);
        goto_error("XCB failed to attach the shm segment"); }

    if (shmctl(shm_o->shmid, IPC_RMID, NULL))
        goto_error("Failed to mark the shm segment to be destroyed "
            "(after the last process detaches): %s", strerror(errno));

    return 0;

err:
    detach_shm(shm_o, conn, xcb);
    return 1;
}

static void detach_shm(xcb_shm_segment_info_t *shm,
    xcb_connection_t *conn, const struct libxcb *xcb)
{
    if (xcb->shm.loaded_) {
        (void) xcb->shm.xcb_shm_detach(conn, shm->shmseg); /* Never fails */
        if (xcb->xcb_flush(conn) <= 0)
            s_log_error("xcb_flush failed!");
    }

    if (shm->shmaddr != (void *)-1 && shm->shmaddr != NULL) {
        shmdt(shm->shmaddr);
        shm->shmaddr = (void *)-1;
    }
    shm->shmid = -1;
    shm->shmseg = -1;
}

static void * malloced_present_thread_fn(void *arg_voidp_)
{
    struct x11_render_software_malloced_present_thread_arg *arg_ = arg_voidp_;

    s_log_trace("THREAD CREATED");

    /* auto memcpy */
    struct x11_render_software_generic_window_info wi = arg_->win_info;
    struct x11_render_shared_malloced_data *const sd = arg_->shared_data;

    xcb_connection_t *const conn = arg_->conn;
    const struct libxcb *const xcb = arg_->xcb;

    struct x11_render_software_ctx *sw_rctx_handle__ = arg_->sw_rctx_handle__;

    u_nfree(&arg_);

    (void) pthread_mutex_lock(&sd->present_request_mutex);
    atomic_store(&sd->present_thread_ready, true);
    goto first_run_entry;

    while (true) {
        (void) pthread_mutex_lock(&sd->present_request_mutex);
first_run_entry:
        (void) pthread_cond_wait(&sd->present_request_cond,
            &sd->present_request_mutex);
        (void) pthread_mutex_unlock(&sd->present_request_mutex);
        if (!atomic_flag_test_and_set(&sd->present_thread_running)) {
            atomic_flag_clear(&sd->present_thread_running);
            break;
        }

        s_assert(atomic_load(&sd->present_request_buffer) != NULL,
            "Present request cond signaled without a buffer to present");

        const struct x11_render_software_malloced_image_buf *const buf =
            atomic_load(&sd->present_request_buffer);

        /* Check that we can at least send a single row of pixels */
        const u64 max_data_size =
            wi.max_request_size - sizeof(xcb_put_image_request_t);
        const u64 rows_per_request = max_data_size / (buf->w * sizeof(pixel_t));
        if (rows_per_request == 0) {
            s_log_error("The maximum request size is too small to send even"
                "1 row of pixels. Dropping the frame.");
            s_log_trace("THREAD: present_pending -> false");
            atomic_store(&sd->present_request_buffer, NULL);
            X11_render_software_finish_frame(sw_rctx_handle__, 1);
            continue;
        }

        const u8 FORMAT = XCB_IMAGE_FORMAT_Z_PIXMAP;
        const xcb_drawable_t DST_DRAWABLE = wi.win_handle;
        const xcb_gcontext_t GC = wi.win_gc;
        const u16 WIDTH = buf->w;
        const i16 DST_X = 0;
        const u8 LEFT_PAD = 0;
        const u8 DEPTH = 24;

        /* Split up the request into smaller chunks so that it can
         * be handle by the X socket connection */
        for (u32 y = 0; y < buf->h; y++) {
            u16 height = buf->h - y;
            if (height > rows_per_request)
                height = rows_per_request;

            const u32 data_len = height * WIDTH * sizeof(pixel_t);
            u8 *const data = (u8 *)buf->buf + (y * WIDTH * sizeof(pixel_t));

            xcb->xcb_put_image(conn, FORMAT, DST_DRAWABLE, GC,
                WIDTH, height, DST_X, y, LEFT_PAD, DEPTH,
                data_len, data);
        }
        (void) xcb->xcb_flush(conn);

        s_log_trace("THREAD: done presenting %p", sd->present_request_buffer);
        s_log_trace("THREAD: present_pending -> false");
        atomic_store(&sd->present_request_buffer, NULL);
        X11_render_software_finish_frame(sw_rctx_handle__, 0);
    }

    s_log_debug("Exiting...");
    pthread_exit(NULL);
}
