#ifndef WINDOW_PRESENT_SOFTWARE_H_
#define WINDOW_PRESENT_SOFTWARE_H_

#ifndef P_INTERNAL_GUARD__
#error This header file is internal to the cgd platform module and is not intended to be used elsewhere
#endif /* P_INTERNAL_GUARD__ */

#include "../window.h"
#include "../thread.h"
#define P_INTERNAL_GUARD__
#include "window-thread.h"
#undef P_INTERNAL_GUARD__
#include <core/int.h>
#include <core/pixel.h>
#include <stdbool.h>
#include <stdatomic.h>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif /* WIN32_LEAN_AND_MEAN */
#include <windows.h>
#include <wingdi.h>

struct render_init_software_req {
    struct window_render_software_ctx *ctx;
    const struct p_window_info *win_info;
};
struct render_present_software_req {
    struct window_render_software_ctx *ctx;
    HWND target_window;
};
struct render_destroy_software_req {
    struct window_render_software_ctx *ctx;
};

struct window_render_software_ctx {
    _Atomic bool initialized_;

    /* This data should only be read from/written to by the window thread
     * while performing a requested `RENDER_*_SOFTWARE` operation */
    struct {
        /* The buffers to which everything is rendered */
        struct window_render_software_buf {
            u32 w, h;
            HBITMAP bmp;
            pixel_t *pixels;

            /* Since we need the request objects to persist on the heap,
             * we have no choice but to declare them here */
            struct render_present_software_req present_req_arg;
            struct window_thread_request present_req;
        } buffers[2];

        /* The GDI device context used to represent the current (front) bitmap
         * during the presentation. `BitBlt` uses it as a handle to the actual
         * in-memory buffer so that it can copy data from it to the screen */
        HDC memdc;

        /* The bitmap that's initially selected in the `memdc`.
         * Since a DC must always have a selected object,
         * this one will be used during cleanup when the buffer bitmaps
         * also get destroyed and there's nothing else
         * to replace them as the DC's object */
        HBITMAP memdc_old_bitmap;
    } window_thread_data_;

    struct window_render_software_buf *front_buf, *back_buf;

    /* The back buffer "representation" returned to the user */
    struct pixel_flat_data user_ret;

    p_mt_mutex_t swap_mutex;
    bool swap_done;
};

i32 render_software_request_init_and_wait(HWND win_handle,
    struct window_render_software_ctx *ctx, const struct p_window_info *info);

struct pixel_flat_data * render_software_swap_buffers_and_request_presentation(
    HWND target_window, struct window_render_software_ctx *ctx
);

void render_software_request_destruction_and_wait(HWND win_handle,
    struct window_render_software_ctx *ctx);

enum window_thread_request_status render_software_handle_window_thread_request(
    enum window_thread_request_type req_type, void *arg
);

#endif /* WINDOW_PRESENT_SOFTWARE_H_ */
