#ifndef EVDEV_H_
#define EVDEV_H_

#ifndef P_INTERNAL_GUARD__
#error This header file is internal to the cgd platform module and is not intended to be used elsewhere
#endif /* P_INTERNAL_GUARD__ */

#include <core/int.h>
#include <core/util.h>
#include <core/vector.h>

#define EVDEV_TYPES_LIST    \
    X_(EVDEV_UNKNOWN)       \
    X_(EVDEV_KEYBOARD)      \
    X_(EVDEV_MOUSE)         \

#define X_(name) name,
enum evdev_type {
    EVDEV_TYPES_LIST
};
#undef X_

#ifndef CGD_BUILDTYPE_RELEASE
#define X_(name) #name,
static const char *const evdev_type_strings[] = {
    EVDEV_TYPES_LIST
};
#undef X_
#endif /* CGD_BUILDTYPE_RELEASE */

#define MAX_EVDEV_NAME_LEN   512
struct evdev {
    i32 fd;
    enum evdev_type type;
    char path[u_FILEPATH_MAX];
    char name[MAX_EVDEV_NAME_LEN];
};

/* `evdev_load_available_devices()` will fail
 * if less than this fraction of devices load successfully */
#define MINIMAL_SUCCESSFUL_EVDEVS_LOADED 0.5f

/* Returns NULL on failure */
VECTOR(struct evdev) evdev_find_and_load_devices(enum evdev_type type);

/* Used internally by `evdev_load_available_devices`.
 * Returns 0 on success and non-zero on failure */
i32 evdev_load(const char *rel_path, struct evdev *out, enum evdev_type type);

#endif /* EVDEV_H_ */
