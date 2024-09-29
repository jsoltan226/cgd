#ifndef R_PUTPIXEL_H_
#define R_PUTPIXEL_H_

#include "rctx.h"
#include <core/pixel.h>

void r_putpixel(struct r_ctx *ctx, i32 x, i32 y, pixel_t val);

#endif /* R_PUTPIXEL_H_ */
