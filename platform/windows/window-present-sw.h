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

/* This is the moudule responsible for presenting software-rendered frames
 * to the window when `P_WINDOW_ACCELERATION_NONE` is used. */

/* Here are the definitions for the arguments of each `window_thread_request`
 * related to this module.
 *
 * To use them, pass a pointer to such a struct as the `arg` field
 * in the `struct window_thread_request`.
 *
 * Note that the pointers must be valid throughout the entire duration
 * of the request & operation! So be careful when allocating them on the stack.
 */

struct render_init_software_req {
    /* The context to be initialized */
    struct window_render_software_ctx *ctx;

    /* The win_info struct containing the window's dimensions.
     * Used to determine the sizes of the buffers that are to be created. */
    const struct p_window_info *win_info;
};
struct render_present_software_req {
    /* The context containing the ALREADY SWAPPED buffers to be presented */
    struct window_render_software_ctx *ctx;

    /* The window to which the frame will be presented */
    HWND target_window;
};
struct render_destroy_software_req {
    /* The context to be destroyed */
    struct window_render_software_ctx *ctx;
};

/* The struct encapulating everything related to presenting
 * a software-rendered frame. */
struct window_render_software_ctx {
    _Atomic bool initialized_; /* Sanity check to avoid double-frees */

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

    /* The front and back buffer pointers (the ones that get swapped).
     * Guarded by `swap_mutex`. */
    struct window_render_software_buf *front_buf, *back_buf;

    /* The variable that's set to signal that the buffers have just been swapped
     * and the new front buffer is ready for scanout.
     *
     * Requesting a presentation without first swapping the buffers
     * will trigger an assertion failure.
     *
     * Also guarded by `swap_mutex`. */
    bool swap_done;

    /* The mutex protecting the buffer swap operation */
    p_mt_mutex_t swap_mutex;

    /* The back buffer "representation" returned to the user */
    struct pixel_flat_data user_ret;
};

/* A wrapper around the `render_init_software` operation.
 * It sends a request to `win_handle` to initialize `ctx`
 * with the parameter `win_info`.
 * It will wait until the request is complete
 * and return 0 on success and non-zero on failure. */
i32 render_software_request_init_and_wait(HWND win_handle,
    struct window_render_software_ctx *ctx,
    const struct p_window_info *win_info);

/* A function that will first swap the buffers in `ctx`,
 * and then send a `render_present_software` request to `win_handle`.
 * It will return as soon as possible (asynchronously) with the new back buffer,
 * or with `NULL` if the request couldn't be sent (dropped frame).
 *
 * To check the result of the presentation itself watch out
 * for `P_EVENT_PAGE_FLIP` events in your event loop. */
struct pixel_flat_data *
render_software_swap_and_request_present(HWND win_handle,
    struct window_render_software_ctx *ctx);

/* A wrapper around the `render_destroy_software` operation.
 * It sends a request to `win_handle` to destroy `ctx`. */
void render_software_request_destruction_and_wait(HWND win_handle,
    struct window_render_software_ctx *ctx);

/* This module's handler for it's `window_thread_request` operations.
 * Used only by the window thread to determine which internal
 * function of this module to call when receiving a request. */
enum window_thread_request_status render_software_handle_window_thread_request(
    enum window_thread_request_type req_type, void *arg
);

#endif /* WINDOW_PRESENT_SOFTWARE_H_ */
