#ifndef WINDOW_X11_RENDER_SW_H_
#define WINDOW_X11_RENDER_SW_H_

#include "../window.h"
#define P_INTERNAL_GUARD__
#include "libxcb-rtld.h"
#undef P_INTERNAL_GUARD__
#include <core/int.h>
#include <core/pixel.h>
#include <stdbool.h>
#include <xcb/xcb.h>
#include <xcb/shm.h>
#include <xcb/present.h>
#include <xcb/xcb_image.h>

#define X11_RENDER_SOFTWARE_FB_TYPE_LIST    \
    X_(X11_SWFB_NULL)                       \
    X_(X11_SWFB_MALLOCED_IMAGE)             \
    X_(X11_SWFB_SHMSEG)                     \
    X_(X11_SWFB_PRESENT_PIXMAP)             \

#define X_(name) name,
enum x11_render_software_fb_type {
    X11_RENDER_SOFTWARE_FB_TYPE_LIST
    X11_SWFB_N_TYPES_
};
#undef X_

#ifndef X11_RENDER_SOFTWARE_FB_TYPE_LIST_DEF__
#undef X11_RENDER_SOFTWARE_FB_TYPE_LIST
#endif /* X11_RENDER_SOFTWARE_FB_TYPE_LIST_DEF__ */

struct x11_render_software_ctx {
    bool initialized_;
    xcb_gcontext_t window_gc;

    struct x11_render_software_buf {
        bool initialized_;
        enum x11_render_software_fb_type type;
        union x11_render_software_fb {
            struct x11_render_software_malloced_image_buf {
                u16 w, h;
                pixel_t *buf;
            } malloced;
            struct x11_render_software_shm_buf {
                u16 w, h;
                xcb_shm_segment_info_t shm_info;
                u8 root_depth;
            } shm;
            struct x11_render_software_present_pixmap_buf {
                u16 w, h;
                xcb_shm_segment_info_t shm_info;
                u8 root_depth;
                xcb_pixmap_t pixmap;
            } present_pixmap;
        } fb;
        struct pixel_flat_data pixbuf;
    } buffers[2], *curr_front_buf, *curr_back_buf;

    struct x11_render_shared_buffer_data {
        struct x11_render_shared_malloced_data {
            _Atomic bool initialized_;
            pthread_t present_thread;
            _Atomic bool present_thread_running;
            _Atomic bool present_thread_ready;

            pthread_mutex_t present_request_mutex;
            pthread_cond_t present_request_cond;
            const struct x11_render_software_malloced_image_buf
                *_Atomic present_request_buffer;

            _Atomic bool *present_pending_p;

            u64 max_request_size;
            xcb_window_t win_handle;
            xcb_gcontext_t gc;
            xcb_connection_t *conn;
            const struct libxcb *xcb;
        } malloced;
        struct x11_render_shared_shm_data {
            _Atomic bool initialized_;
            const struct xcb_query_extension_reply_t *_Atomic ext_data;
            _Atomic u64 blit_request_sequence_number;
        } shm;
        struct x11_render_shared_present_data {
            _Atomic bool initialized_;
            const struct xcb_query_extension_reply_t *_Atomic ext_data;
            _Atomic xcb_present_event_t event_context_id;
            _Atomic u32 serial;
        } present;
    } shared_buf_data;

    _Atomic bool present_pending;
};

i32 X11_render_init_software(struct x11_render_software_ctx *sw_rctx,
    u16 win_w, u16 win_h, u8 root_depth, u64 max_request_size,
    xcb_window_t win_handle, xcb_connection_t *conn, const struct libxcb *xcb,
    bool *o_vsync_supported);

struct pixel_flat_data * X11_render_present_software(
    struct x11_render_software_ctx *sw_rctx, xcb_window_t win_handle,
    xcb_connection_t *conn, const struct libxcb *xcb,
    enum p_window_present_mode present_mode
);

void X11_render_destroy_software(struct x11_render_software_ctx *sw_rctx,
    xcb_connection_t *conn, xcb_window_t win_hadle, const struct libxcb *xcb);

void X11_render_software_finish_frame(struct x11_render_software_ctx *sw_rctx,
    bool status);

#endif /* WINDOW_X11_RENDER_SW_H_ */
