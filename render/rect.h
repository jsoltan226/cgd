#ifndef R_RECT_H_
#define R_RECT_H_

#include "rctx.h"

void r_draw_rect(struct r_ctx *ctx,
    const i32 u_x, const i32 u_y,
    const i32 u_w, const i32 u_h);

void r_fill_rect(struct r_ctx *ctx,
    const i32 u_x, const i32 u_y,
    const i32 u_w, const i32 u_h);

#endif /* R_RECT_H_ */
