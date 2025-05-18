#define P_INTERNAL_GUARD__
#define X11_RENDER_SOFTWARE_FB_TYPE_LIST_DEF__
#include "window-x11-present-sw.h"
#undef X11_RENDER_SOFTWARE_FB_TYPE_LIST_DEF__
#undef P_INTERNAL_GUARD__
#define P_INTERNAL_GUARD__
#include "libxcb-rtld.h"
#undef P_INTERNAL_GUARD__
#define P_INTERNAL_GUARD__
#include "window-x11.h"
#undef P_INTERNAL_GUARD__
#include "../event.h"
#include "../window.h"
#include <core/int.h>
#include <core/log.h>
#include <core/util.h>
#include <errno.h>
#include <string.h>
#include <stdatomic.h>
#include <sys/shm.h>
#include <sys/ipc.h>

#define MODULE_NAME "window-x11-present-sw"

static i32 software_init_buffer_malloced(
    struct x11_render_software_malloced_image_buf *buffer_o,
    struct pixel_flat_data *pixbuf_o,
    u16 win_w, u16 win_h, u8 root_depth,
    xcb_window_t win_handle, xcb_connection_t *conn, const struct libxcb *xcb
);
static i32 software_init_buffer_shm(
    struct x11_render_software_shm_buf *buffer_o,
    struct pixel_flat_data *pixbuf_o,
    u16 win_w, u16 win_h, u8 root_depth,
    xcb_window_t win_handle, xcb_connection_t *conn, const struct libxcb *xcb
);
static i32 software_init_buffer_pixmap(
    struct x11_render_software_present_pixmap_buf *buffer_o,
    struct pixel_flat_data *pixbuf_o,
    u16 win_w, u16 win_h, u8 root_depth,
    xcb_window_t win_handle, xcb_connection_t *conn, const struct libxcb *xcb
);

static i32 software_present_malloced(
    const struct x11_render_software_malloced_image_buf *buf,
    xcb_connection_t *conn, xcb_window_t win_handle, xcb_gcontext_t window_gc,
    const struct libxcb *xcb
);
static i32 software_present_shm(
    const struct x11_render_software_shm_buf *buf,
    xcb_connection_t *conn, xcb_window_t win_handle, xcb_gcontext_t window_gc,
    const struct libxcb *xcb
);
static i32 software_present_pixmap(
    const struct x11_render_software_present_pixmap_buf *buf,
    struct x11_render_software_present_ext_data *ext_data,
    xcb_connection_t *conn, xcb_window_t win_handle, xcb_gcontext_t window_gc,
    const struct libxcb *xcb, enum p_window_present_mode present_mode
);

static void software_destroy_buffer_malloced(
    struct x11_render_software_malloced_image_buf *buf,
    xcb_connection_t *conn, const struct libxcb *xcb
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
    struct x11_render_software_present_ext_data *present_ext_data,
    u16 win_w, u16 win_h, u8 root_depth,
    xcb_connection_t *conn, xcb_window_t win_handle, const struct libxcb *xcb);

static i32 init_present_extension_data(const struct libxcb *xcb,
    struct x11_render_software_present_ext_data *ext_data,
    xcb_connection_t *conn, xcb_window_t win_handle);
static void destroy_present_extension_data(const struct libxcb *xcb,
    struct x11_render_software_present_ext_data *ext_data,
    xcb_connection_t *conn, xcb_window_t win_handle);

static i32 attach_shm(xcb_shm_segment_info_t *shm_o, u32 w, u32 h,
    xcb_connection_t *conn, const struct libxcb *xcb);
static void detach_shm(xcb_shm_segment_info_t *shm,
    xcb_connection_t *conn, const struct libxcb *xcb);

i32 X11_render_init_software(struct x11_render_software_ctx *sw_rctx,
    u16 win_w, u16 win_h, u8 root_depth, xcb_window_t win_handle,
    xcb_connection_t *conn, const struct libxcb *xcb,
    bool *o_vsync_supported)
{
    *o_vsync_supported = false;
    sw_rctx->initialized_ = true;
    atomic_init(&sw_rctx->present_pending, false);

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

    for (u32 i = 0; i < 2; i++) {
        if (do_init_buffer(&sw_rctx->buffers[i], &sw_rctx->present,
            win_w, win_h, root_depth, conn, win_handle, xcb))
        {
            s_log_error("Failed to initialize buffer %u in any way", i);
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

    *o_vsync_supported =
        sw_rctx->buffers[0].type == X11_SWFB_PRESENT_PIXMAP &&
        sw_rctx->buffers[1].type == X11_SWFB_PRESENT_PIXMAP;

    sw_rctx->curr_front_buf = &sw_rctx->buffers[0];
    sw_rctx->curr_back_buf = &sw_rctx->buffers[1];

    return 0;
}

struct pixel_flat_data * X11_render_present_software(
    struct x11_render_software_ctx *sw_rctx, xcb_window_t win_handle,
    xcb_connection_t *conn, const struct libxcb *xcb,
    enum p_window_present_mode present_mode
)
{
    /* We assume that `present_mode` has already been validated */

    if (atomic_load(&sw_rctx->present_pending)) {
        s_log_warn("Another frame presentation already scheduled; "
            "dropping this one!");
        return &sw_rctx->curr_back_buf->pixbuf;
    }
    atomic_store(&sw_rctx->present_pending, true);
    /* Resets after the page flip completes */

    /* Swap the buffers */
    struct x11_render_software_buf *const tmp = sw_rctx->curr_front_buf;
    sw_rctx->curr_front_buf = sw_rctx->curr_back_buf;
    sw_rctx->curr_back_buf = tmp;

    struct x11_render_software_buf *const curr_buf = sw_rctx->curr_front_buf;
    switch (curr_buf->type) {
    case X11_SWFB_NULL:
        s_log_fatal("Attempt to present an uninitialized buffer");
    case X11_SWFB_MALLOCED_IMAGE:
        software_present_malloced(&curr_buf->fb.malloced,
            conn, win_handle, sw_rctx->window_gc, xcb);
        /* Since vsync isn't supported here, we don't care
         * about any data races that might cause tearing */
        X11_render_software_finish_frame(sw_rctx, 0);
        break;
    case X11_SWFB_SHMSEG:
        software_present_shm(&curr_buf->fb.shm,
            conn, win_handle, sw_rctx->window_gc, xcb);
        /* Same as above */
        X11_render_software_finish_frame(sw_rctx, 0);
        break;
    case X11_SWFB_PRESENT_PIXMAP:
        software_present_pixmap(&curr_buf->fb.present_pixmap, &sw_rctx->present,
            conn, win_handle, sw_rctx->window_gc, xcb, present_mode);
        /* In this case, `X11_render_software_finish_frame` gets called
         * by the listener thread when it receives a PRESENT_COMPLETE event */
        break;
    default:
        s_log_fatal("Invalid buffer type %d", curr_buf->type);
    }

    if (xcb->xcb_flush(conn) <= 0)
        s_log_error("xcb_flush failed!");

    return &sw_rctx->curr_back_buf->pixbuf;
}

void X11_render_destroy_software(struct x11_render_software_ctx *sw_rctx,
    xcb_connection_t *conn, xcb_window_t win_handle, const struct libxcb *xcb)
{
    if (sw_rctx == NULL || !sw_rctx->initialized_)
        return;

    if (atomic_load(&sw_rctx->present_pending)) {
        s_log_warn("Forcing buffer destruction while it might be in use");
        atomic_store(&sw_rctx->present_pending, false);
    }

    for (u32 i = 0; i < 2; i++) {
        if (!sw_rctx->initialized_)
            continue;

        struct x11_render_software_buf *const curr_buf = &sw_rctx->buffers[i];
        switch (curr_buf->type) {
        case X11_SWFB_NULL: break;
        case X11_SWFB_MALLOCED_IMAGE:
            software_destroy_buffer_malloced(&curr_buf->fb.malloced,
                conn, xcb);
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

    if (sw_rctx->present.initialized_)
        destroy_present_extension_data(xcb, &sw_rctx->present, conn, win_handle);

    if (sw_rctx->window_gc != XCB_NONE) {
        xcb->xcb_free_gc(conn, sw_rctx->window_gc);
        sw_rctx->window_gc = XCB_NONE;
    }

    sw_rctx->curr_back_buf = sw_rctx->curr_front_buf = NULL;
    sw_rctx->initialized_ = false;
}

void X11_render_software_finish_frame(struct x11_render_software_ctx *sw_rctx,
    bool status)
{
    atomic_store(&sw_rctx->present_pending, false);
    p_event_send(&(const struct p_event) {
        .type = P_EVENT_PAGE_FLIP,
        .info.page_flip_status = status
    });
}

static i32 software_init_buffer_malloced(
    struct x11_render_software_malloced_image_buf *buffer_o,
    struct pixel_flat_data *pixbuf_o,
    u16 win_w, u16 win_h, u8 root_depth,
    xcb_window_t win_handle, xcb_connection_t *conn, const struct libxcb *xcb
)
{
    (void) win_handle;

    memset(buffer_o, 0, sizeof(struct x11_render_software_malloced_image_buf));
    memset(pixbuf_o, 0, sizeof(struct pixel_flat_data));

    /* Create the image */
    buffer_o->image = xcb->xcb_image_create_native(conn,
        win_w, win_h, XCB_IMAGE_FORMAT_Z_PIXMAP, root_depth,
        NULL, 0, NULL); /* Let xcb calculate the size */
    if (buffer_o->image == NULL) {
        s_log_error("Failed to create the XCB image");
        return 1;
    }

    /* Fill in the pixel data struct */
    pixbuf_o->w = buffer_o->image->width;
    pixbuf_o->h = buffer_o->image->height;
    pixbuf_o->buf = buffer_o->image->base;

    return 0;
}

static i32 software_init_buffer_shm(
    struct x11_render_software_shm_buf *buffer_o,
    struct pixel_flat_data *pixbuf_o,
    u16 win_w, u16 win_h, u8 root_depth,
    xcb_window_t win_handle, xcb_connection_t *conn, const struct libxcb *xcb
)
{
    (void) win_handle;

    memset(buffer_o, 0, sizeof(struct x11_render_software_shm_buf));
    memset(pixbuf_o, 0, sizeof(struct pixel_flat_data));

    buffer_o->root_depth = root_depth;
    buffer_o->w = win_w;
    buffer_o->h = win_h;

    /* Initialize the shm segment */
    if (attach_shm(&buffer_o->shm_info, win_w, win_h, conn, xcb)) {
        s_log_error("Failed to initialize the shared memory segment");
        return 1;
    }

    /* Fill in the pixel data struct */
    pixbuf_o->w = win_w;
    pixbuf_o->h = win_h;
    pixbuf_o->buf = (pixel_t *)buffer_o->shm_info.shmaddr;

    return 0;
}

static i32 software_init_buffer_pixmap(
    struct x11_render_software_present_pixmap_buf *buffer_o,
    struct pixel_flat_data *pixbuf_o,
    u16 win_w, u16 win_h, u8 root_depth,
    xcb_window_t win_handle, xcb_connection_t *conn, const struct libxcb *xcb
)
{
    memset(buffer_o, 0, sizeof(struct x11_render_software_present_pixmap_buf));
    memset(pixbuf_o, 0, sizeof(struct pixel_flat_data));

    if (X11_check_shm_extension(conn, xcb)) {
        s_log_error("The MIT-SHM extension is not available!");
        return -1;
    }

    if (X11_check_present_extension(conn, xcb)) {
        s_log_error("The X Present extension is not available!");
        return -1;
    }

    if (attach_shm(&buffer_o->shm_info, win_w, win_h, conn, xcb)) {
        s_log_error("Failed to initialize the shared memory segment");
        return 1;
    }

    buffer_o->w = win_w;
    buffer_o->h = win_h;
    buffer_o->root_depth = root_depth;

    buffer_o->pixmap = xcb->xcb_generate_id(conn);

    /* This request never fails */
    (void) xcb->shm.xcb_shm_create_pixmap(
        conn, buffer_o->pixmap, win_handle, win_w, win_h,
        root_depth, buffer_o->shm_info.shmseg, 0
    );

    pixbuf_o->w = win_w;
    pixbuf_o->h = win_h;
    pixbuf_o->buf = (pixel_t *)buffer_o->shm_info.shmaddr;

    return 0;
}

static i32 software_present_malloced(
    const struct x11_render_software_malloced_image_buf *buf,
    xcb_connection_t *conn, xcb_window_t win_handle, xcb_gcontext_t window_gc,
    const struct libxcb *xcb
)
{
    (void) xcb->xcb_image_put(conn, win_handle, window_gc, buf->image, 0, 0, 0);
    return 0;
}

static i32 software_present_shm(
    const struct x11_render_software_shm_buf *buf,
    xcb_connection_t *conn, xcb_window_t win_handle, xcb_gcontext_t window_gc,
    const struct libxcb *xcb
)
{
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

    (void) xcb->shm.xcb_shm_put_image(
        conn, win_handle, window_gc,
        TOTAL_SRC_IMAGE_W, TOTAL_SRC_IMAGE_H, SRC_X, SRC_Y, SRC_W, SRC_H,
        DST_X, DST_Y, DST_DEPTH, DST_IMAGE_FORMAT,
        SEND_BLIT_COMPLETE_EVENT, SHMSEG, SRC_START_OFFSET
    );

    return 0;
}

static i32 software_present_pixmap(
    const struct x11_render_software_present_pixmap_buf *buf,
    struct x11_render_software_present_ext_data *ext_data,
    xcb_connection_t *conn, xcb_window_t win_handle, xcb_gcontext_t gc,
    const struct libxcb *xcb, enum p_window_present_mode present_mode
)
{
    (void) gc;

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

    (void) xcb->present.xcb_present_pixmap(conn, win_handle, buf->pixmap,
        ++ext_data->serial, VALID_PIXMAP_REGION, WINDOW_UPDATE_REGION,
        DST_OFFSET_X, DST_OFFSET_Y, TARGET_CRTC, WAIT_FENCE, IDLE_FENCE,
        OPTIONS, TARGET_MONITOR_COUNTER, DIVISOR, REMAINDER,
        NOTIFIES_LEN, NOTIFIES);

    return 0;
}

static void software_destroy_buffer_malloced(
    struct x11_render_software_malloced_image_buf *buf,
    xcb_connection_t *conn, const struct libxcb *xcb
)
{
    (void) conn;

    if (buf->image != XCB_NONE) {
        xcb->xcb_image_destroy(buf->image);
        buf->image = XCB_NONE;
    }
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
        xcb_void_cookie_t vc = xcb->xcb_free_pixmap_checked(conn, buf->pixmap);
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
    struct x11_render_software_present_ext_data *present_ext_data,
    u16 win_w, u16 win_h, u8 root_depth,
    xcb_connection_t *conn, xcb_window_t win_handle, const struct libxcb *xcb)
{
    buf->initialized_ = true;
    buf->type = X11_SWFB_NULL;

    if (software_init_buffer_pixmap(&buf->fb.present_pixmap,
            &buf->pixbuf, win_w, win_h, root_depth,
            win_handle, conn, xcb
        ) == 0)
    {
        /* Now that we have a pixmap buffer, to be able to use it
         * we need to init the struct containing the data that's shared
         * between all buffers (related to the present extension) */
        if (!present_ext_data->initialized_ &&
            init_present_extension_data(xcb, present_ext_data, conn, win_handle)
        ) {
            s_log_error("Failed to init X Present extension data");
            destroy_present_extension_data(xcb, present_ext_data,
                conn, win_handle);
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
            &buf->pixbuf, win_w, win_h, root_depth,
            win_handle, conn, xcb
        ) == 0)
    {
        buf->type = X11_SWFB_SHMSEG;
        return 0;
    } else {
        s_log_warn("Failed to create window framebuffer with shm, "
            "falling back to manually malloc'd buffers");
        software_destroy_buffer_shm(&buf->fb.shm, conn, xcb);
    }

    if (software_init_buffer_malloced(&buf->fb.malloced,
            &buf->pixbuf, win_w, win_h, root_depth,
            win_handle, conn, xcb
        ) == 0)
    {
        buf->type = X11_SWFB_MALLOCED_IMAGE;
        return 0;
    } else {
        s_log_error("Failed to initialize manually allocated buffers");
        return 1;
    }

    s_log_fatal("impossible outcome");
}

static i32 init_present_extension_data(const struct libxcb *xcb,
    struct x11_render_software_present_ext_data *ext_data,
    xcb_connection_t *conn, xcb_window_t win_handle)
{
    if (ext_data->initialized_) {
        s_log_warn("X Present extension data already initialized; "
            "not doing anything.");
        return 0;
    }
    ext_data->initialized_ = true;

    ext_data->serial = 0;
    ext_data->ext_data =
        xcb->xcb_get_extension_data(conn, xcb->present.xcb_present_id);
    s_assert(ext_data->ext_data != NULL,
        "The X Present extension data should be loaded & cached by now");

    ext_data->event_context_id = xcb->xcb_generate_id(conn);

    xcb_void_cookie_t vc = xcb->present.xcb_present_select_input_checked(
        conn, ext_data->event_context_id, win_handle,
        XCB_PRESENT_EVENT_MASK_COMPLETE_NOTIFY
    );
    xcb_generic_error_t *e = NULL;
    if (e = xcb->xcb_request_check(conn, vc), e != NULL) {
        s_log_error("xcb_present_select_input failed");
        u_nfree(&e);
        ext_data->event_context_id = XCB_NONE;
        return 1;
    }

    return 0;
}

static void destroy_present_extension_data(const struct libxcb *xcb,
    struct x11_render_software_present_ext_data *ext_data,
    xcb_connection_t *conn, xcb_window_t win_handle)
{
    if (!ext_data->initialized_)
        return;
    ext_data->initialized_ = false;

    if (ext_data->event_context_id != XCB_NONE
        && xcb->present._voidp_xcb_present_select_input_checked != NULL
    ) {
        /* Delete the event context */
        xcb_void_cookie_t vc = xcb->present.xcb_present_select_input_checked(
            conn, ext_data->event_context_id, win_handle, 0
        );
        xcb_generic_error_t *e = NULL;
        if (e = xcb->xcb_request_check(conn, vc), e != NULL) {
            s_log_error("xcb_present_select_input_checked failed");
            u_nfree(&e);
        }
        ext_data->event_context_id = XCB_NONE;
    }
    ext_data->ext_data = NULL;
    ext_data->serial = 0;
}

static i32 attach_shm(xcb_shm_segment_info_t *shm_o, u32 w, u32 h,
    xcb_connection_t *conn, const struct libxcb *xcb)
{
    if (X11_check_shm_extension(conn, xcb)) {
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
        goto_error("XCB failed to attach the shm segment");
    }

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
