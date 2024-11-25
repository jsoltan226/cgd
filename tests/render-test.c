#include "platform/event.h"
#include <core/log.h>
#include <core/util.h>
#include <platform/time.h>
#include <platform/window.h>
#include <render/rctx.h>
#include <stdlib.h>
#include <stdbool.h>

#define MODULE_NAME "render-test"

#define WINDOW_TITLE        ((const unsigned char *)MODULE_NAME)
#define WINDOW_AREA         (&(const rect_t) { 0, 0, 500, 500 })
#define WINDOW_FLAGS        (P_WINDOW_POS_CENTERED_XY)

static struct p_window *win = NULL;
static struct r_ctx *rctx = NULL;

int cgd_main(int argc, char **argv)
{
    s_configure_log(LOG_DEBUG, stdout, stderr);

    win = p_window_open(WINDOW_TITLE, WINDOW_AREA, WINDOW_FLAGS);
    if (win == NULL)
        goto_error("Failed to open a window. Stop.");

    rctx = r_ctx_init(win, R_TYPE_DEFAULT, 0);
    if (rctx == NULL)
        goto_error("Failed to initialize the renderer. Stop");

    struct p_event ev;
    bool running = true;
    while (running) {
        while (p_event_poll(&ev)) {
            if (ev.type == P_EVENT_QUIT)
                running = false;
        }
    }

    r_ctx_destroy(&rctx);
    p_window_close(&win);
    return EXIT_SUCCESS;

err:
    if (rctx != NULL) r_ctx_destroy(&rctx);
    if (win != NULL) p_window_close(&win);
    return EXIT_FAILURE;
}
