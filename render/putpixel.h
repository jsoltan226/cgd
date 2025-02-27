#ifndef R_PUTPIXEL_H_
#define R_PUTPIXEL_H_

#include <core/pixel.h>

void r_putpixel_rgba(struct pixel_flat_data *data, u32 x, u32 y, pixel_t val);
void r_putpixel_bgra(struct pixel_flat_data *data, u32 x, u32 y, pixel_t val);

#endif /* R_PUTPIXEL_H_ */
