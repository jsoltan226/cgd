#include "line.h"
#include <core/math.h>
#include <core/util.h>
#include <core/shapes.h>
#define R_INTERNAL_GUARD__
#include "rctx-internal.h"
#undef R_INTERNAL_GUARD__
#define R_INTERNAL_GUARD__
#include "putpixel-fast.h"
#undef R_INTERNAL_GUARD__

#define MODULE_NAME "line"

void r_draw_line(struct r_ctx *rctx, vec2d_t start, vec2d_t end)
{
    u_check_params(rctx != NULL);

    /* Cut off any part of the line that
     * would extend beyond the framebuffer */
    start.x = u_clamp(start.x, 0.f, (f32)(rctx->pixels.w - 1));
    start.y = u_clamp(start.y, 0.f, (f32)(rctx->pixels.w - 1));
    end.x = u_clamp(end.x, 0.f, (f32)(rctx->pixels.h - 1));
    end.y = u_clamp(end.y, 0.f, (f32)(rctx->pixels.h - 1));

    i32 dx = end.x - start.x;
    i32 dy = end.y - start.y;
    if (dx == 0 || dy == 0) return;

    i32 step_x = 1, step_y = 1;

    if (dx < 0) {
        step_x = -1;
        dx = -dx;
    }
    if (dy < 0) {
        step_y = -1;
        dy = -dy;
    }

    i32 err = dx - dy;

    i32 x = start.x;
    i32 y = start.y;

    if (abs(dx) > abs(dy)) {
        /* Shallow slope (|dx| > |dy|) - Increment x more frequently */
        for (; x != end.x; x += step_x) {
            r_putpixel_fast_matching_pixelfmt_(
                rctx->pixels.buf,
                x, y, rctx->pixels.w,
                rctx->current_color
            );

            err -= dy;
            if (err < 0) {
                y += step_y;
                err += abs(dx);  /* Reset error when overshooting */
            }
        }
    } else {
        /* Steep slope (|dy| > |dx|) - Increment y more frequently */
        err = dy / 2;  /* Reset err to be based on dy for this case */
        for (; y != end.y; y += step_y) {
            r_putpixel_fast_matching_pixelfmt_(
                rctx->pixels.buf,
                x, y, rctx->pixels.w,
                rctx->current_color);

            err -= dx;
            if (err < 0) {
                x += step_x;
                err += abs(dy);  /* Reset error when overshooting */
            }
        }
    }
}
