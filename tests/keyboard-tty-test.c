#define _GNU_SOURCE
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include "core/log.h"
#include "platform/keyboard.h"
#include "platform/window.h"

#define MODULE_NAME "keyboard-tty-test"

#define SECONDS_TO_LIVE 5
#define FPS 60

int main(void) {
    s_set_log_level(LOG_DEBUG);
    s_set_log_out_filep(stdout);
    s_set_log_err_filep(stderr);
    
    struct p_window *win = p_window_open(
        (const unsigned char *)MODULE_NAME,
        &(rect_t) { 0 },
        P_WINDOW_TYPE_FRAMEBUFFER
    );
    if (win == NULL) {
        s_log_error("Failed to open the window. Stop.");
        return EXIT_FAILURE;
    }

    struct p_keyboard *kb = p_keyboard_init(win);
    if (kb == NULL) {
        s_log_error("Failed to initialize the keyboard. Stop.");
        p_window_close(win);
        return EXIT_FAILURE;
    }

    u32 i = 0;
    while (!p_keyboard_get_key(kb, KB_KEYCODE_Q)->pressed && i++ < SECONDS_TO_LIVE * FPS) {
        p_keyboard_update(kb);
        usleep(1000000 / FPS);
    }

    p_keyboard_destroy(kb);
    p_window_close(win);

    return EXIT_SUCCESS;
}
