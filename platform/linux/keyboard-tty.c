#define _GNU_SOURCE
#define P_INTERNAL_GUARD__
#include "keyboard-tty.h"
#undef P_INTERNAL_GUARD__
#define P_INTERNAL_GUARD__
#include "tty.h"
#undef P_INTERNAL_GUARD__
#include "../keyboard.h"
#include <core/log.h>
#include <core/util.h>
#include <core/ansi-esc-sequences.h>
#include <string.h>
#include <unistd.h>

#define MODULE_NAME "keyboard-tty"

static i32 get_next_key(struct keyboard_tty *kb);
static enum p_keyboard_keycode parse_buffered_sequence(char buf[MAX_ESC_SEQUENCE_LEN]);
static enum p_keyboard_keycode parse_standard_char(char c);

i32 keyboard_tty_init(struct keyboard_tty *kb)
{
    memset(kb, 0, sizeof(struct keyboard_tty));
    if (tty_ctx_init(&kb->ttydev_ctx, NULL))
        goto_error("Failed to initialize the tty device");

    if (tty_set_raw_mode(&kb->ttydev_ctx))
        goto_error("Failed to set the tty to raw mode");

    return 0;

err:
    keyboard_tty_destroy(kb);
    return 1;
}

void keyboard_tty_update_all_keys(struct keyboard_tty *kb,
    pressable_obj_t *pobjs)
{
    enum p_keyboard_keycode kc = 0;
    bool key_updated[P_KEYBOARD_N_KEYS] = { 0 };

    /* Update the keys that were pressed */
    while (kc = get_next_key(kb), kc != -1) {
        pressable_obj_update(&pobjs[kc], true);
        key_updated[kc] = true;
    }

    /* Update the keys that were not pressed */
    for (u32 i = 0; i < P_KEYBOARD_N_KEYS; i++) {
        if (key_updated[i]) continue;
        pressable_obj_update(&pobjs[i], false);
    }
}

void keyboard_tty_destroy(struct keyboard_tty *kb)
{
    if (kb == NULL)
        return;

    tty_ctx_cleanup(&kb->ttydev_ctx);
    memset(kb->esc_seq_buf, 0, sizeof(kb->esc_seq_buf));
}

static i32 get_next_key(struct keyboard_tty *kb)
{
    if (kb->esc_seq_buf[0])
        return parse_buffered_sequence(kb->esc_seq_buf);

    char c;
    if (read(kb->ttydev_ctx.fd, &c, 1) != 1)
        return -1; /* Either `read()` failed (-1)
                      or no more data is available (0 [bytes read]) */

    if (c == es_ESC_chr) {
        kb->esc_seq_buf[0] = es_ESC_chr;
        if (read(kb->ttydev_ctx.fd,
                kb->esc_seq_buf + 1, MAX_ESC_SEQUENCE_LEN - 1) <= 0)
            return -1; /* Same as above */

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
