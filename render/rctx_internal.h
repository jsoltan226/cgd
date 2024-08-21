#ifndef RCTX_INTERNAL_H_
#define RCTX_INTERNAL_H_
#include "platform/window.h"
#ifndef R_INTERNAL_GUARD__
#error This header is internal to the cgd renderer module and is not intented to be used elsewhere
#endif /* R_INTERNAL_GUARD__ */

#include "rctx.h"
#include "core/pixel.h"

struct r_ctx {
    enum r_type type;

    struct p_window *win;
    struct p_window_meta win_meta;

    struct pixel_flat_data pixels;

    pixel_t current_color;
};

#endif /* RCTX_INTERNAL_H_ */
