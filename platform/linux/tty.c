#define _GNU_SOURCE
#define P_INTERNAL_GUARD__
#include "tty.h"
#undef P_INTERNAL_GUARD__
#include <core/int.h>
#include <core/log.h>
#include <core/util.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>

#define MODULE_NAME "tty"

static i32 apply_tty_config(i32 fd, const struct termios *cfg);
static i32 termios_cmp(const struct termios *t1, const struct termios *t2);

i32 tty_ctx_init(struct tty_ctx *o, const char *tty_dev_path)
{
    u_check_params(o != NULL);
    if (tty_dev_path == NULL) tty_dev_path = TTY_DEFAULT_DEV_PATH;

    o->fd = -1;
    memset(&o->orig_termios, 0, sizeof(struct termios));
    memset(&o->curr_termios, 0, sizeof(struct termios));

    /* Open the tty device */
    o->fd = open(tty_dev_path, O_RDWR | O_NONBLOCK | O_CLOEXEC);
    if (o->fd == -1)
        goto_error("Failed to open %s: %s", tty_dev_path, strerror(errno));

    /* Perform sanity checks */
    if (!isatty(o->fd))
        goto_error("%s is not a tty!", tty_dev_path);

    /* Get the terminal configuration */
    if (tcgetattr(o->fd, &o->curr_termios))
        goto_error("Failed to get the current tty configuration: %s",
            strerror(errno));

    memcpy(&o->orig_termios, &o->curr_termios, sizeof(struct termios));

    s_log_debug("Successfully initialized tty device \"%s\"", tty_dev_path);

    return 0;
err:
    tty_ctx_cleanup(o);
    return 1;
}

i32 tty_set_raw_mode(struct tty_ctx *ctx)
{
    u_check_params(ctx != NULL && ctx->fd >= 0);

    /* Set the tty to raw mode so that we get input on a per-character basis
     * instead of a per-line basis */
    ctx->curr_termios.c_lflag &= ~(ICANON | ECHO);
    ctx->curr_termios.c_iflag &= ~(IXON | ICRNL);

    return apply_tty_config(ctx->fd, &ctx->curr_termios);
}

void tty_restore(struct tty_ctx *ctx)
{
    u_check_params(ctx != NULL && ctx->fd >= 0);

    (void) tcsetattr(ctx->fd, TCSAFLUSH, &ctx->orig_termios);

    if (apply_tty_config(ctx->fd, &ctx->orig_termios)) {
        s_log_error("Failed to restore the original tty configuration");
        s_log_info("If you encounter any problems with your terminal, "
            "try the command `stty sane`.");
        return;
    }

    s_log_debug("Restored original terminal configuration");
}

void tty_ctx_cleanup(struct tty_ctx *ctx)
{
    if (ctx == NULL)
        return;

    if (ctx->fd >= 0) {
        tty_restore(ctx);

        if (close(ctx->fd)) {
            s_log_error("Failed to close the tty file descriptor: %s",
                strerror(errno));
        }
        ctx->fd = -1;
    }

    memset(&ctx->curr_termios, 0, sizeof(struct termios));
    memset(&ctx->orig_termios, 0, sizeof(struct termios));
}

static i32 apply_tty_config(i32 fd, const struct termios *cfg)
{
    (void) tcsetattr(fd, TCSANOW, cfg);

    struct termios tmp;
    if (tcgetattr(fd, &tmp)) {
        s_log_error("Failed to get the current tty configuration: %s",
            strerror(errno));
        return 1;
    }

    if (termios_cmp(&tmp, cfg)) {
        s_log_error("The current and new terminal configurations don't match!");
        return 1;
    }

    return 0;
}

static i32 termios_cmp(const struct termios *t1, const struct termios *t2)
{
    if (t1->c_iflag != t2->c_iflag) return 1;
    if (t1->c_oflag != t2->c_oflag) return 1;
    if (t1->c_cflag != t2->c_cflag) return 1;
    if (t1->c_lflag != t2->c_lflag) return 1;

    for (u32 i = 0; i < NCCS; i++) {
        if (t1->c_cc[i] != t2->c_cc[i])
            return 1;
    }

    return 0;
}
