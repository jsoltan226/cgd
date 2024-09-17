#ifndef CORE_MATH_H_
#define CORE_MATH_H_

#include "int.h"
#include "shapes.h"
#include <stdbool.h>

#define u_min(a, b) (a < b ? a : b)
#define u_max(a, b) (a > b ? a : b)
#define u_clamp(x, min, max) (u_min(u_max(x, min), max))

/* The simplest collision checking implementation;
 * returns true if 2 rectangles overlap
 */
static inline bool u_collision(const rect_t *r1, const rect_t *r2)
{
    return (
        r1->x <= r2->x + r2->w &&
        r1->x + r1->w >= r2->x &&
        r1->y <= r2->y + r2->h &&
        r1->y + r1->h >= r2->y
    );
}

#endif /* CORE_MATH_H_ */
