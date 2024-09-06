#include "util.h"
#include "core/shapes.h"

#define MODULE_NAME "util"

bool u_collision(const rect_t *r1, const rect_t *r2)
{
    return (
        r1->x <= r2->x + r2->w &&
        r1->x + r1->w >= r2->x &&
        r1->y <= r2->y + r2->h &&
        r1->y + r1->h >= r2->y
    );
}
