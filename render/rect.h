#ifndef R_RECT_H_
#define R_RECT_H_

#include "rctx.h"
#include "core/shapes.h"

void r_draw_rect(struct r_ctx *ctx, const rect_t *r);
void r_fill_rect(struct r_ctx *ctx, const rect_t *r);

#endif /* R_RECT_H_ */
