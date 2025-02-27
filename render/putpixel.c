#include "putpixel.h"
#include <core/pixel.h>
#include <stdlib.h>
#define R_INTERNAL_GUARD__
#include "putpixel-fast.h"
#undef R_INTERNAL_GUARD__

/* This is just `r_putpixel_fast_` with argument validation */

void r_putpixel_rgba(struct pixel_flat_data *data, u32 x, u32 y, pixel_t val)
{
    if (data == NULL || data->buf == NULL || x >= data->w || y >= data->h)
        return;

    r_putpixel_fast_matching_pixelfmt_(data->buf, x, y, data->w, val);
}

void r_putpixel_bgra(struct pixel_flat_data *data, u32 x, u32 y, pixel_t val)
{
    if (data == NULL || data->buf == NULL || x >= data->w || y >= data->h)
        return;

    r_putpixel_fast_(data->buf, x, y, data->w, val, BGRA32);
}
