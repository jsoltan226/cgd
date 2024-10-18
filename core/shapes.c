#include "shapes.h"
#include "math.h"
#include <stdlib.h>

void rect_clip(rect_t *r, const rect_t * restrict max)
{
    if (r == NULL || max == NULL) return;

    r->x = u_clamp(r->x, max->x, max->x + max->w - 1);
    r->y = u_clamp(r->y, max->y, max->y + max->h - 1);
    r->w = u_clamp(r->w, 0, max->w - r->x);
    r->h = u_clamp(r->h, 0, max->h - r->y);
}
