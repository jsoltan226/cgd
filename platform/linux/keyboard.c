#include "../keyboard.h"
#include "core/int.h"
#include "core/log.h"
#include "core/util.h"
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>

extern void cfmakeraw(struct termios *termios_p); /* should be in termios.h, but clangd is stupid */

#define MODULE_NAME "keyboard"

struct p_keyboard {
    enum { KB_MODE_DEV_INPUT, KB_MODE_TTY } mode;
    
    union {
        struct p_keyboard_devinput {

        } devinput;
        struct p_keyboard_tty {
            i32 fd;
            struct termios orig_termios, termios;
            bool is_orig_termios_initialized_;
        } tty;
    };
};

static i32 devinput_keyboard_init(struct p_keyboard_devinput *kb_devinput);
static i32 tty_keyboard_init(struct p_keyboard_tty *kb_tty);
static void devinput_keyboard_destroy(struct p_keyboard_devinput *kb_devinput);
static void tty_keyboard_destroy(struct p_keyboard_tty *kb_tty);

struct p_keyboard * p_keyboard_init()
{
    struct p_keyboard *kb = calloc(1, sizeof(struct p_keyboard));
    s_assert(kb != NULL, "calloc() failed for struct p_keyboard");

    kb->mode = KB_MODE_DEV_INPUT;
    if (devinput_keyboard_init(&kb->devinput))
        s_log_warn("Failed to set up keyboard using /dev/input, falling back to tty mode");

    kb->mode = KB_MODE_TTY;
    if (tty_keyboard_init(&kb->tty)) {
        s_log_error("Failed to set up keyboard using tty. No other methods left.");
        p_keyboard_destroy(kb);
        return NULL;
    }
    

    return kb;
}

void p_keyboard_destroy(struct p_keyboard *kb)
{
    if (kb == NULL) return;
    
    free(kb);
}

static i32 devinput_keyboard_init(struct p_keyboard_devinput *kb_devinput)
{

    if (kb_devinput == NULL)
        goto err;

    return 0;

err:
    devinput_keyboard_destroy(kb_devinput);
    return 1;
}

#undef MODULE_NAME
#define MODULE_NAME "keyboard-tty"
#define TTYDEV_FILEPATH "/dev/tty"

static i32 tty_keyboard_init(struct p_keyboard_tty *kb_tty)
{
    kb_tty->fd = -1;
    kb_tty->is_orig_termios_initialized_ = false;

    /* Open the tty device */
    kb_tty->fd = open(TTYDEV_FILEPATH, O_RDONLY);
    if (kb_tty->fd == -1)
        goto_error("Failed to open %s: %s", TTYDEV_FILEPATH, strerror(errno));

    /* Perform sanity checks */
    if (!isatty(kb_tty->fd))
        goto_error("%s is not a tty!", TTYDEV_FILEPATH);

    /* Get the terminal configuration */
    tcgetattr(kb_tty->fd, &kb_tty->orig_termios);

    /* Save terminal configuration before modifying it
     * so that it can be restored during cleanup */
    memcpy(&kb_tty->orig_termios, &kb_tty->termios, sizeof(struct termios));
    kb_tty->is_orig_termios_initialized_ = true;

    /* Set the tty to raw mode so that we get input on a per-character basis
     * instead of a per-line basis */
    cfmakeraw(&kb_tty->termios);
    if (tcsetattr(kb_tty->fd, TCSANOW, &kb_tty->termios))
        goto_error("Failed to set tty to raw mode: %s", strerror(errno));
    


    return 0;

err:
    tty_keyboard_destroy(kb_tty);
    return 1;
}

static void devinput_keyboard_destroy(struct p_keyboard_devinput *kb_devinput)
{
    if (kb_devinput == NULL) return;
}

static void tty_keyboard_destroy(struct p_keyboard_tty *kb_tty)
{
    if (kb_tty->fd >= 0) {
        /* Try to restore the original configuration upon encountering an error */
        if (kb_tty->is_orig_termios_initialized_) {
            tcsetattr(kb_tty->fd, TCSANOW, &kb_tty->orig_termios);
        }

        close(kb_tty->fd);
    }
}
