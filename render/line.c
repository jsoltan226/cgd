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

void r_draw_line(struct r_ctx *rctx, const vec2d_t start, const vec2d_t end)
{
    u_check_params(rctx != NULL);

    /* Ensure x0 <= x1 and y0 <= y1 */
    vec2d_t final_start, final_end;
    if (start.x > end.x) {
        final_start.x = end.x;
        final_end.x = start.x;
    } else {
        final_start.x = start.x;
        final_end.x = end.x;
    }
    if (start.y > end.y) {
        final_start.y = end.y;
        final_end.y = start.y;
    } else {
        final_start.y = start.y;
        final_end.y = end.y;
    }

    const i32 dx = final_end.x - final_start.x;
    const i32 dy = final_end.y - final_start.y;

    i32 err = dx - dy;

    i32 x = final_start.x;
    i32 y = final_start.y;

    if (dx > dy) {
        /* Shallow slope (|dx| > |dy|) - Increment x more frequently */
        for (; x <= final_end.x; x++) {
            r_putpixel_fast_matching_pixelfmt_(
                rctx->pixels.buf,
                x, y, rctx->pixels.w,
                rctx->current_color
            );

            err -= dy;
            if (err < 0) {
                y++;
                err += dx;  /* Reset error when overshooting */
            }
        }
    } else {
        /* Steep slope (|dy| > |dx|) - Increment y more frequently */
        err = dy / 2;  /* Reset err to be based on dy for this case */
        for (; y != final_end.y; y++) {
            r_putpixel_fast_matching_pixelfmt_(
                rctx->pixels.buf,
                x, y, rctx->pixels.w,
                rctx->current_color);

            err -= dx;
            if (err < 0) {
                x++;
                err += dy;  /* Reset error when overshooting */
            }
        }
    }
}
