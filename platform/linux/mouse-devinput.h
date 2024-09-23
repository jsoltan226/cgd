#ifndef MOUSE_DEVINPUT_H_
#define MOUSE_DEVINPUT_H_

#ifndef P_INTERNAL_GUARD__
#error This header file is internal to the cgd platform module and is not intended to be used elsewhere
#endif /* P_INTERNAL_GUARD__ */

#include "core/int.h"
#include "../mouse.h"

struct mouse_devinput {
};

i32 mouse_devinput_init(struct mouse_devinput *mouse, u32 flags);

void mouse_devinput_get_state(struct mouse_devinput *mouse,
    struct p_mouse_state *o, bool update);

void mouse_devinput_destroy(struct mouse_devinput *mouse);

#endif /* MOUSE_DEVINPUT_H_ */
