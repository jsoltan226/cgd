#ifndef R_RECT_H_
#define R_RECT_H_

#include "rctx.h"

void r_draw_rect(struct r_ctx *ctx,
    const i32 x, const i32 y, const i32 w, const i32 h);

void r_fill_rect(struct r_ctx *ctx,
    const i32 x, const i32 y, const i32 w, const i32 h);

#endif /* R_RECT_H_ */
