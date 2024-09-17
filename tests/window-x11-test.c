#include "core/log.h"
#include "core/shapes.h"
#include "core/util.h"
#include "platform/window.h"
#include "render/rctx.h"
#include "render/rect.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define MODULE_NAME "window-x11-test"

#define WINDOW_W    500
#define WINDOW_H    500
#define WINDOW_FLAGS (P_WINDOW_TYPE_NORMAL | P_WINDOW_POS_CENTERED_XY)

#define RECT_W 100
#define RECT_H 100
#define RECT_X ((WINDOW_W - RECT_W) / 2)
#define RECT_Y ((WINDOW_H - RECT_H) / 2)

static const color_RGBA32_t RECT_TEST_COLORS[] = {
    { 0,    0,      0   },
    { 255,  0,      0   },
    { 0,    255,    0   },
    { 0,    0,      255 },
    { 255,  255,    0   },
    { 0,    255,    255 },
    { 255,  0,      255 },
    { 255,  255,    255 },
};

struct p_window *win = NULL;
struct r_ctx *rctx = NULL;

static void sprint_num_with_pad(u8 num, char o[4]);

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

    rctx = r_ctx_init(win, R_TYPE_SOFTWARE, 0);
    if (rctx == NULL)
        goto_error("Failed to init renderer. Stop");

    char R[4], G[4], B[4], A[4];
    for (u32 i = 0; i < u_arr_size(RECT_TEST_COLORS); i++) {
        sprint_num_with_pad(RECT_TEST_COLORS[i].r, R);
        sprint_num_with_pad(RECT_TEST_COLORS[i].g, G);
        sprint_num_with_pad(RECT_TEST_COLORS[i].b, B);
        sprint_num_with_pad(RECT_TEST_COLORS[i].a, A);

        s_log_debug("Current color: |R: %s | G: %s | B: %s | A: %s |", R, G, B, A);

        r_ctx_set_color(rctx, RECT_TEST_COLORS[i]);
        r_fill_rect(rctx, &(rect_t) { RECT_X, RECT_Y, RECT_W, RECT_H });
        r_flush(rctx);
        sleep(1);
    }

    p_window_close(win);
    r_ctx_destroy(rctx);
    return EXIT_SUCCESS;

err:
    if (win != NULL) p_window_close(win);
    if (rctx != NULL) r_ctx_destroy(rctx);
    return EXIT_FAILURE;
}

static void sprint_num_with_pad(u8 num, char o[4])
{
    memcpy(o, (char []){ ' ', ' ', ' ', '\0' }, 4);
    char tmp[4] = { 0 };

    snprintf(tmp, 4, "%u", num);

    /* Exclude the null terminator */
    strncpy(o, tmp, strlen(tmp));
}
