#include <core/log.h>
#include <core/util.h>
#include <core/shapes.h>
#include <platform/ptime.h>
#include <platform/event.h>
#include <platform/window.h>
#include <render/rctx.h>
#include <render/rect.h>
#include <stdlib.h>
#include <string.h>

#define MODULE_NAME "window-test"
#include "log-util.h"

#define WINDOW_W    500
#define WINDOW_H    500
#define WINDOW_FLAGS (P_WINDOW_POS_CENTERED_XY | P_WINDOW_NO_ACCELERATION)

#define RECT_W 100
#define RECT_H 100
#define RECT_X ((WINDOW_W - RECT_W) / 2)
#define RECT_Y ((WINDOW_H - RECT_H) / 2)

static struct p_window *win = NULL;
static struct r_ctx *rctx = NULL;

int cgd_main(int argc, char **argv)
{
    (void) argc;
    (void) argv;
    if (test_log_setup())
        return EXIT_FAILURE;

    win = p_window_open(
        MODULE_NAME,
        &(rect_t) { 0, 0, WINDOW_W, WINDOW_H },
        WINDOW_FLAGS
    );
    if (win == NULL)
        goto_error("Failed to open window. Stop.");

    rctx = r_ctx_init(win, R_TYPE_SOFTWARE, 0);
    if (rctx == NULL)
        goto_error("Failed to init renderer. Stop");

    for (u32 i = 0; i < 10; i++) {
        s_log_trace("Swap buffers");
        r_flush(rctx);
    }

    struct p_event ev = { 0 };
    u32 counter = 0;
    while ((ev.type != P_EVENT_PAGE_FLIP || ev.info.page_flip_status != 0)
        && counter < 100) {
        p_time_msleep(1);
        p_event_poll(&ev);
        counter++;
    }
    if (counter >= 100)
        s_log_error("Waiting for page flip event failed - timed out");

    for (u32 i = 0; i < 10; i++) {
        s_log_trace("Swap buffers & wait");
        r_flush(rctx);
        memset(&ev, 0, sizeof(struct p_event));
        counter = 0;
        while ((ev.type != P_EVENT_PAGE_FLIP || ev.info.page_flip_status != 0)
            && counter < 100)
        {
            p_time_msleep(1);
            p_event_poll(&ev);
            counter++;
        }
        if (counter >= 100)
            s_log_error("Waiting for page flip event failed - timed out");
    }

    color_RGBA32_t color = { 0, 0, 0, 255 };
    memset(&ev, 0, sizeof(struct p_event));
    for (u32 i = 0; i < 255; i++) {
        while (p_event_poll(&ev)) {
            if (ev.type == P_EVENT_QUIT)
                goto quit;
        }

        color.r += 1;
        color.g += 2;
        color.b += 3;
        r_reset(rctx);
        r_ctx_set_color(rctx, color);
        r_fill_rect(rctx, RECT_X, RECT_Y, RECT_W, RECT_H);
        r_flush(rctx);
        p_time_usleep(16000);
    }

quit:
    r_ctx_destroy(&rctx);
    p_window_close(&win);
    return EXIT_SUCCESS;

err:
    if (rctx != NULL) r_ctx_destroy(&rctx);
    if (win != NULL) p_window_close(&win);
    return EXIT_FAILURE;
}
