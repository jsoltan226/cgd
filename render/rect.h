#ifndef R_RECT_H_
#define R_RECT_H_

#include "rctx.h"
#include "core/shapes.h"

void r_draw_rect(struct r_ctx *ctx, i32 x, i32 y, i32 w, i32 h);
void r_fill_rect(struct r_ctx *ctx, i32 x, i32 y, i32 w, i32 h);

#endif /* R_RECT_H_ */
