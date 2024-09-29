#ifndef MOUSE_EVDEV_H_
#define MOUSE_EVDEV_H_

#ifndef P_INTERNAL_GUARD__
#error This header file is internal to the cgd platform module and is not intended to be used elsewhere
#endif /* P_INTERNAL_GUARD__ */

#include "../mouse.h"
#include <core/int.h>
#include <core/datastruct/vector.h>
#include <poll.h>

struct mouse_evdev {
    VECTOR(struct evdev) mouse_devs;
    VECTOR(struct pollfd) poll_fds;
};

i32 mouse_evdev_init(struct mouse_evdev *mouse, u32 flags);

void mouse_evdev_update(struct p_mouse *mouse);

void mouse_evdev_destroy(struct mouse_evdev *mouse);

#endif /* MOUSE_EVDEV_H_ */
