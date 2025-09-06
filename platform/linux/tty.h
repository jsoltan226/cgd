#ifndef TTY_H_
#define TTY_H_

#include <core/int.h>
#include <stdbool.h>
#include <termios.h>

#include <platform/common/guard.h>

/* A helper struct that stores the information necessary
 * to manipulate the terminal */
struct tty_ctx {
    i32 fd;
    struct termios orig_termios;
    struct termios curr_termios;
};

#define TTY_DEFAULT_DEV_PATH "/dev/tty"

/* Initializes `o` with the tty device `tty_dev_path`.
 * If `tty_dev_path` is `NULL`, the default tty device is opened.
 * Returns 0 on success and non-zero on failure. */
i32 tty_ctx_init(struct tty_ctx *o, const char *tty_dev_path);

/* Sets the tty in `ctx` to raw mode.
 * Returns 0 on success and non-zero on failure. */
i32 tty_set_raw_mode(struct tty_ctx *ctx);

/* Restores the original configuration of the tty in `ctx`.
 * Returns 0 on success and non-zero on failure.
 *
 * Note: You don't have to call this function to clean up the tty,
 * it's done automatically by `tty_ctx_cleanup`. */
void tty_restore(struct tty_ctx *ctx);

/* Cleans up and destroys `ctx` */
void tty_ctx_cleanup(struct tty_ctx *ctx);

#endif /* TTY_H_ */
