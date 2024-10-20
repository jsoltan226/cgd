#include "putpixel.h"
#include <platform/window.h>
#include <stdlib.h>
#define R_INTERNAL_GUARD__
#include "putpixel-fast.h"
#undef R_INTERNAL_GUARD__

void r_putpixel_rgba(struct pixel_flat_data *data, i32 x, i32 y, pixel_t val)
{
    if (data == NULL || data->buf == NULL || x >= data->w || y >= data->h)
        return;

    r_putpixel_fast_(data->buf, x, y, data->w, val,
        P_WINDOW_RGBA8888);
}

void r_putpixel_bgra(struct pixel_flat_data *data, i32 x, i32 y, pixel_t val)
{
    if (data == NULL || data->buf == NULL || x >= data->w || y >= data->h)
        return;

    r_putpixel_fast_(data->buf, x, y, data->w, val,
        P_WINDOW_BGRA8888);
}
