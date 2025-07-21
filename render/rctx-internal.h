#ifndef RCTX_INTERNAL_H_
#define RCTX_INTERNAL_H_
#ifndef R_INTERNAL_GUARD__
#error This header is internal to the cgd renderer module and is not intented to be used elsewhere
#endif /* R_INTERNAL_GUARD__ */

#include "rctx.h"
#include <core/pixel.h>
#include <core/shapes.h>
#include <platform/window.h>
#include <platform/thread.h>

struct r_ctx {
    enum r_type type;

    struct p_window *win;
    struct p_window_info win_info;

    struct pixel_flat_data *curr_buf;

    rect_t pixels_rect;

    pixel_t current_color;

    p_mt_thread_t thread;
    struct r_ctx_thread_info {
        _Atomic bool running;
        p_mt_cond_t cond;
        p_mt_mutex_t mutex;
    } thread_info;

    u64 total_frames, dropped_frames;
};

extern void renderer_main(void *arg);

#endif /* RCTX_INTERNAL_H_ */
