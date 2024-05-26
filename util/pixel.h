#ifndef PIXEL_H_
#define PIXEL_H_

#include "util.h"
#include "int.h"

typedef u_Color pixel_t;

struct pixel_data {
    u8 **data;
    u32 w, h;
};

static const pixel_t EMPTY_PIXEL = { 0 };
static const pixel_t BLACK_PIXEL = { 0, 0, 0, 255 };
static const pixel_t WHITE_PIXEL = { 255, 255, 255, 255 };

void destroy_pixel_data(struct pixel_data *pixel_data);

#endif /* PIXEL_H_ */
