#ifndef U_SHAPES_H_
#define U_SHAPES_H_

#include "int.h"
#include <assert.h>

typedef struct {
    f32 x, y;
} vec2d_t;

typedef struct {
    f32 x, y, z;
} vec3d_t;

typedef struct {
    i32 x, y, w, h;
} rect_t;

typedef struct {
    u8 r, g, b, a;
} color_RGBA32_t;

static_assert(sizeof(color_RGBA32_t) == 4,
    "The size of color_RGBA32_t must be 4 bytes (32 bits)");

#endif /* U_SHAPES_H_ */
