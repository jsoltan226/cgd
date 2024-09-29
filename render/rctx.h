#ifndef RCTX_H_
#define RCTX_H_

#include <core/shapes.h>
#include <platform/window.h>

struct r_ctx;

enum r_type {
    R_TYPE_DEFAULT,
    R_TYPE_SOFTWARE,
    R_TYPE_OPENGL,
    R_TYPE_VULKAN,
};

struct r_ctx * r_ctx_init(struct p_window *win, enum r_type type, u32 flags);
void r_ctx_destroy(struct r_ctx *rctx);

void r_ctx_set_color(struct r_ctx *ctx, color_RGBA32_t color);

void r_reset(struct r_ctx *ctx);
void r_flush(struct r_ctx *ctx);

#endif /* RCTX_H_ */
