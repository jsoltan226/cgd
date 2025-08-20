#ifndef MOUSE_EVDEV_H_
#define MOUSE_EVDEV_H_

#include <platform/common/guard.h>

#include "../mouse.h"
#include <core/int.h>
#include <core/vector.h>
#include <poll.h>

struct mouse_evdev {
    VECTOR(struct evdev) mouse_devs;
    VECTOR(struct pollfd) poll_fds;
};

i32 mouse_evdev_init(struct mouse_evdev *mouse);

void mouse_evdev_update(struct p_mouse *mouse);

void mouse_evdev_destroy(struct mouse_evdev *mouse);

#endif /* MOUSE_EVDEV_H_ */
