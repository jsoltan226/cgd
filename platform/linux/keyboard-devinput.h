#ifndef KEYBOARD_DEVINPUT_H_
#define KEYBOARD_DEVINPUT_H_

#ifndef P_INTERNAL_GUARD__
#error This header file is internal to the cgd platform module and is not intended to be used elsewhere
#endif /* P_INTERNAL_GUARD__ */

#include "core/int.h"
#include "../keyboard.h"

#define MAX_KEYBOARD_DEV_FILEPATH_LEN   512

/* The key file is /sys/class/input/inputXX/capabilities/key */
#define SYSFS_INPUT_DIR "/sys/class/input"
#define SYSFS_INPUT_DEV_DIR_PREFIX "input"
#define SYSFS_INPUT_DEV_KEY_FILE "capabilities/key"
#define SYSFS_INPUT_DEV_KEY_MAX_LEN 512

struct keyboard_devinput {
    char path[MAX_KEYBOARD_DEV_FILEPATH_LEN];
};

i32 devinput_keyboard_init(struct keyboard_devinput *kb);
void devinput_keyboard_destroy(struct keyboard_devinput *kb);

enum p_keyboard_keycode devinput_keyboard_next_key(struct keyboard_devinput *kb);

#endif /* KEYBOARD_DEVINPUT_H_ */
