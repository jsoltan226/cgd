#ifndef KEYBOARD_TTY_H_
#define KEYBOARD_TTY_H_

#include <platform/common/guard.h>

#define P_INTERNAL_GUARD__
#include "tty.h"
#undef P_INTERNAL_GUARD__
#include <core/int.h>
#include <core/pressable-obj.h>

struct keyboard_tty {
    struct tty_ctx ttydev_ctx;

#define MAX_ESC_SEQUENCE_LEN 256
    char esc_seq_buf[MAX_ESC_SEQUENCE_LEN + 2];
};

i32 keyboard_tty_init(struct keyboard_tty *kb);

void keyboard_tty_update_all_keys(struct keyboard_tty *kb,
    pressable_obj_t *pobjs);

void keyboard_tty_destroy(struct keyboard_tty *kb);

#endif /* KEYBOARD_TTY_H_ */
