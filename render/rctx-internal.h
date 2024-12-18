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
    struct p_window_meta win_meta;

    struct pixel_flat_data buffers[2];
    struct pixel_flat_data *render_buffer, *present_buffer;

    rect_t pixels_rect;

    pixel_t current_color;

    p_mt_thread_t thread;
    struct r_ctx_thread_info {
        p_mt_cond_t running;
        p_mt_mutex_t mutex;
    } thread_info;
};

extern void renderer_main(void *arg);

#endif /* RCTX_INTERNAL_H_ */
