#ifndef KEYBOARD_TTY_H_
#define KEYBOARD_TTY_H_

#ifndef P_INTERNAL_GUARD__
#error This header file is internal to the cgd platform module and is not intended to be used elsewhere
#endif /* P_INTERNAL_GUARD__ */

#include "core/int.h"
#include <termios.h>
#include <stdbool.h>

struct keyboard_tty {
    i32 fd;
    struct termios orig_termios, termios;
    bool is_orig_termios_initialized_;

#define MAX_ESC_SEQUENCE_LEN 256
    char esc_seq_buf[MAX_ESC_SEQUENCE_LEN + 2];
};

i32 tty_keyboard_init(struct keyboard_tty *kb);
i32 tty_keyboard_next_key(struct keyboard_tty *kb);
void tty_keyboard_destroy(struct keyboard_tty *kb);

#endif /* KEYBOARD_TTY_H_ */
