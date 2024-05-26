#ifndef U_SHAPES_H_
#define U_SHAPES_H_

#include "int.h"

typedef union {
    f32 x, y;
    f32 pos[2];
} vec2d_t;

typedef union {
    f32 x, y;
    f32 pos[2];
} vec3d_t;

typedef struct {
    i32 x, y, w, h;
} rect_t;

typedef struct {
    u8 r, g, b, a;
} color_RGBA32_t;

#endif /* U_SHAPES_H_ */
