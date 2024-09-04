#include "../keyboard.h"
#include "core/log.h"
#include <sys/types.h>
#define P_INTERNAL_GUARD__
#include "keyboard-tty.h"
#undef P_INTERNAL_GUARD__
#include <fcntl.h>
#include "core/util.h"
#include <termios.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include "core/ansi-esc-sequences.h"

#define MODULE_NAME "keyboard-tty"

static enum p_keyboard_keycode parse_buffered_sequence(char buf[MAX_ESC_SEQUENCE_LEN]);
static enum p_keyboard_keycode parse_standard_char(char c);

#define TTYDEV_FILEPATH "/dev/tty"
i32 tty_keyboard_init(struct keyboard_tty *kb)
{
    memset(kb, 0, sizeof(struct keyboard_tty));

    kb->fd = -1;
    kb->is_orig_termios_initialized_ = false;

    /* Open the tty device */
    kb->fd = open(TTYDEV_FILEPATH, O_RDONLY | O_NONBLOCK);
    if (kb->fd == -1)
        goto_error("Failed to open %s: %s", TTYDEV_FILEPATH, strerror(errno));

    /* Perform sanity checks */
    if (!isatty(kb->fd))
        goto_error("%s is not a tty!", TTYDEV_FILEPATH);

    /* Get the terminal configuration */
    tcgetattr(kb->fd, &kb->orig_termios);

    /* Save terminal configuration before modifying it
     * so that it can be restored during cleanup */
    memcpy(&kb->termios, &kb->orig_termios, sizeof(struct termios));
    kb->is_orig_termios_initialized_ = true;

    /* Set the tty to raw mode so that we get input on a per-character basis
     * instead of a per-line basis */
    kb->termios.c_lflag &= ~(ICANON | ECHO);
    kb->termios.c_iflag &= ~(IXON | ICRNL);
    tcsetattr(kb->fd, TCSANOW, &kb->termios);
    
    /* Make sure that the changes were all successfully applied */
    struct termios tmp = { 0 };
    tcgetattr(kb->fd, &tmp);
    if (memcmp(&tmp, &kb->termios, sizeof(struct termios)))
        goto_error("Failed to set tty to raw mode: %s", strerror(errno));

    return 0;

err:
    tty_keyboard_destroy(kb);
    return 1;
}

i32 tty_keyboard_next_key(struct keyboard_tty *kb)
{
    if (kb->esc_seq_buf[0])
        return parse_buffered_sequence(kb->esc_seq_buf);

    char c;
    if (read(kb->fd, &c, 1) != 1)
        return -1; /* Either `read()` failed (-1) or no more data is available (0 [bytes read]) */

    if (c == es_ESC_chr) {
        kb->esc_seq_buf[0] = es_ESC_chr;
        read(kb->fd, kb->esc_seq_buf + 1, MAX_ESC_SEQUENCE_LEN - 1);
        return parse_buffered_sequence(kb->esc_seq_buf);
    }

    return parse_standard_char(c);
}

void tty_keyboard_destroy(struct keyboard_tty *kb)
{
    if (kb->fd >= 0) {
        if (kb->is_orig_termios_initialized_) {
            tcsetattr(kb->fd, TCSANOW, &kb->orig_termios);
            s_log_debug("Restored original terminal configuration");
        }

        close(kb->fd);
    }
}

static enum p_keyboard_keycode parse_buffered_sequence(char buf[MAX_ESC_SEQUENCE_LEN])
{
    if (!(buf[0] == es_ESC_chr && buf[1] == '[')) {
        enum p_keyboard_keycode ret = parse_standard_char(buf[0]);
        memmove(buf, buf + 1, MAX_ESC_SEQUENCE_LEN - 1);
        buf[MAX_ESC_SEQUENCE_LEN - 1] = 0;
        return ret;
    }
    
    char *chr_p = buf;

    char tmp_buf[MAX_ESC_SEQUENCE_LEN] = { 0 };
    strncpy(tmp_buf, es_CSI, MAX_ESC_SEQUENCE_LEN);
    chr_p += 2;

    u32 i = 2;
    do {
        char c = *chr_p;

        if (c == ';' || (c >= '0' && c <= '9') || c == '[') {
            tmp_buf[i++] = c;
        }
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '~') {
            tmp_buf[i++] = c;
            break;
        } else {
            break;
        }
    } while (*(++chr_p) && i < MAX_ESC_SEQUENCE_LEN);

    memmove(buf, buf + i, MAX_ESC_SEQUENCE_LEN - i);
    memset(buf + MAX_ESC_SEQUENCE_LEN - i, 0, i);

    if (!strncmp(tmp_buf, es_CUR_UP, MAX_ESC_SEQUENCE_LEN)) return KB_KEYCODE_ARROWUP;
    if (!strncmp(tmp_buf, es_CUR_DOWN, MAX_ESC_SEQUENCE_LEN)) return KB_KEYCODE_ARROWDOWN;
    if (!strncmp(tmp_buf, es_CUR_LEFT, MAX_ESC_SEQUENCE_LEN)) return KB_KEYCODE_ARROWLEFT;
    if (!strncmp(tmp_buf, es_CUR_RIGHT, MAX_ESC_SEQUENCE_LEN)) return KB_KEYCODE_ARROWRIGHT;

    return -1;
}

static enum p_keyboard_keycode parse_standard_char(char c)
{
    if (c >= '0' && c <= '9') return KB_KEYCODE_DIGIT0 + (c - '0');   
    if (c >= '0' && c <= '9') return KB_KEYCODE_DIGIT0 + (c - '0');   
    if (c >= 'a' && c <= 'z') return KB_KEYCODE_A + (c - 'a');
    if (c >= 'A' && c <= 'Z') return KB_KEYCODE_A + (c - 'A'); // shift mask not implemented yet

    switch (c) {
        case ' ': return KB_KEYCODE_SPACE;
        case es_ESC_chr: return KB_KEYCODE_ESCAPE;
        case es_CARRIAGE_RETURN_chr: case es_LINEFEED_chr:
            return KB_KEYCODE_ENTER;
        default:
            return -1;
    }
}
