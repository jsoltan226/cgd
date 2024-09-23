#include "core/int.h"
#include "../mouse.h"
#define P_INTERNAL_GUARD__
#include "mouse-devinput.h"
#undef P_INTERNAL_GUARD__

i32 mouse_devinput_init(struct mouse_devinput *mouse, u32 flags)
{
    return 1;
}

void mouse_devinput_get_state(struct mouse_devinput *mouse,
    struct p_mouse_state *o, bool update)
{
}

void mouse_devinput_destroy(struct mouse_devinput *mouse)
{
}
