#ifndef PIXEL_H_
#define PIXEL_H_

#include "shapes.h"
#include "int.h"

typedef color_RGBA32_t pixel_t;

struct pixel_flat_data {
    pixel_t *buf;
    u32 w, h;
};

struct pixel_row_data {
    pixel_t **rows;
    u32 w, h;
};

#define EMPTY_PIXEL ((pixel_t) { 0 })
#define BLACK_PIXEL ((pixel_t) { 0, 0, 0, 255 })
#define WHITE_PIXEL ((pixel_t) { 255, 255, 255, 255 })

i32 pixel_row_data_init(struct pixel_row_data *out, u32 w, u32 h);
void pixel_row_data_destroy(struct pixel_row_data *data);

void pixel_data_row2flat(struct pixel_row_data *in, struct pixel_flat_data *out);
void pixel_data_flat2row(struct pixel_flat_data *in, struct pixel_row_data *out);

#endif /* PIXEL_H_ */
