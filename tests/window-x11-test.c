#include "core/log.h"
#include "core/util.h"
#include "platform/window.h"
#include <stdlib.h>
#include <unistd.h>

#define MODULE_NAME "window-x11-test"

#define WINDOW_W    500
#define WINDOW_H    500
#define WINDOW_FLAGS (P_WINDOW_TYPE_NORMAL | P_WINDOW_POS_CENTERED_XY)

struct p_window *win = NULL;

int main(void)
{
    s_set_log_level(LOG_DEBUG);
    s_set_log_out_filep(stdout);
    s_set_log_err_filep(stderr);

    win = p_window_open(
        (const unsigned char *)MODULE_NAME,
        &(rect_t) { 0, 0, WINDOW_W, WINDOW_H },
        WINDOW_FLAGS
    );
    if (win == NULL)
        goto_error("Failed to open window. Stop.");

    sleep(5);

    p_window_close(win);
    return EXIT_SUCCESS;

err:
    if (win != NULL) p_window_close(win);
    return EXIT_FAILURE;
}
