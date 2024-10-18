#include "../keyboard.h"
#include <core/log.h>
#include <core/util.h>
#include <core/ansi-esc-sequences.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <sys/types.h>
#define P_INTERNAL_GUARD__
#include "keyboard-tty.h"
#undef P_INTERNAL_GUARD__

#define MODULE_NAME "keyboard-tty"

static i32 tty_keyboard_next_key(struct keyboard_tty *kb);
static enum p_keyboard_keycode parse_buffered_sequence(char buf[MAX_ESC_SEQUENCE_LEN]);
static enum p_keyboard_keycode parse_standard_char(char c);

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

    /* Set the terminal to raw mode */
    if (tty_keyboard_set_term_raw_mode(kb->fd, &kb->orig_termios,
            &kb->is_orig_termios_initialized_))
        goto err;

    return 0;

err:
    tty_keyboard_destroy(kb);
    return 1;
}

void tty_keyboard_update_all_keys(struct keyboard_tty *kb,
    pressable_obj_t *pobjs)
{
    enum p_keyboard_keycode kc = 0;
    bool key_updated[P_KEYBOARD_N_KEYS] = { 0 };

    /* Update the keys that were pressed */
    while (kc = tty_keyboard_next_key(kb), kc != -1) {
        pressable_obj_update(&pobjs[kc], true);
        key_updated[kc] = true;
    }

    /* Update the keys that were not pressed */
    for (u32 i = 0; i < P_KEYBOARD_N_KEYS; i++) {
        if (key_updated[i]) continue;
        pressable_obj_update(&pobjs[i], false);
    }
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
    memset(kb, 0, sizeof(struct keyboard_tty));
    kb->fd = -1;
}

i32 tty_keyboard_set_term_raw_mode(i32 fd, struct termios *orig_termios_o,
    bool *is_orig_termios_initialized_o)
{
    u_check_params(orig_termios_o != NULL && fd != -1);
    struct termios tmp = { 0 };
    
    /* Get the terminal configuration */
    tcgetattr(fd, orig_termios_o);

    /* Save terminal configuration before modifying it
     * so that it can be restored during cleanup */
    memcpy(&tmp, orig_termios_o, sizeof(struct termios));
    if (is_orig_termios_initialized_o != NULL)
        *is_orig_termios_initialized_o = true;

    /* Set the tty to raw mode so that we get input on a per-character basis
     * instead of a per-line basis */
    tmp.c_lflag &= ~(ICANON | ECHO);
    tmp.c_iflag &= ~(IXON | ICRNL);
    tcsetattr(fd, TCSANOW, &tmp);
    
    /* Make sure that the changes were all successfully applied */
    struct termios chk = { 0 };
    tcgetattr(fd, &chk);
    if (memcmp(&tmp, &chk, sizeof(struct termios))) {
        s_log_error("Failed to set tty to raw mode: %s", strerror(errno));
        return 1;
    }

    return 0;
}

static i32 tty_keyboard_next_key(struct keyboard_tty *kb)
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
