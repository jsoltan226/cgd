#ifndef KEYBOARD_TTY_H_
#define KEYBOARD_TTY_H_

#ifndef P_INTERNAL_GUARD__
#error This header file is internal to the cgd platform module and is not intended to be used elsewhere
#endif /* P_INTERNAL_GUARD__ */

#include <core/int.h>
#include <core/pressable-obj.h>
#include <termios.h>
#include <stdbool.h>

#define TTYDEV_FILEPATH "/dev/tty"

struct keyboard_tty {
    i32 fd;
    struct termios orig_termios, termios;
    bool is_orig_termios_initialized_;

#define MAX_ESC_SEQUENCE_LEN 256
    char esc_seq_buf[MAX_ESC_SEQUENCE_LEN + 2];
};

i32 tty_keyboard_init(struct keyboard_tty *kb);

void tty_keyboard_update_all_keys(struct keyboard_tty *kb,
    pressable_obj_t *pobjs);

void tty_keyboard_destroy(struct keyboard_tty *kb);

/* Exposed here for use in `window-fb`.
 * Returns 0 on success and non-zero on failure.
 *
 * `fd` should be an open file descriptor of the tty.
 *
 * `orig_termios_o` should contain a pointer to a termios struct to which
 *  the original tty configuration will be copied,
 *  so that we cab restore it later.
 *
 * `is_orig_termios_initialized_o` should contain a pointer to a bool
 * which will be set once the terminal configuration is
 * loaded into `orig_termios_o`
 * (in the `keyboard_tty` struct that would be `is_orig_termios_initialized_`).
 * Can be NULL.
 */
i32 tty_keyboard_set_term_raw_mode(i32 fd, struct termios *orig_termios_o,
    bool *is_orig_termios_initialized_o);

#endif /* KEYBOARD_TTY_H_ */
