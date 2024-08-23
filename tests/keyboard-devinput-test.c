#include "core/log.h"
#include "platform/keyboard.h"
#include "platform/window.h"

int main(void)
{
    s_set_log_level(LOG_DEBUG);
    s_set_log_out_filep(stdout);
    s_set_log_err_filep(stderr);

    struct p_window *win = p_window_open(0, 0, 0, 0, WINDOW_FRAMEBUFFER);
    struct p_keyboard *kb = p_keyboard_init(win);
    p_keyboard_destroy(kb);
    p_window_close(win);
    return 0;
}
