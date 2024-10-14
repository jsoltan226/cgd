#include "asset-loader/img-type.h"
#include "core/pixel.h"
#include "core/shapes.h"
#include "render/line.h"
#include <core/log.h>
#include <core/util.h>
#include <render/rctx.h>
#include <render/rect.h>
#include <render/surface.h>
#include <platform/time.h>
#include <platform/event.h>
#include <platform/window.h>
#include <asset-loader/asset.h>
#include <asset-loader/io-PNG.h>
#include <asset-loader/plugin.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define MODULE_NAME "surface-test"

#define WINDOW_TITLE (const unsigned char *)MODULE_NAME
#define WINDOW_RECT (const rect_t) { 0, 0, 500, 500 }
#define WINDOW_FLAGS (P_WINDOW_TYPE_AUTO | P_WINDOW_POS_CENTERED_XY)

#define FPS 60

#define SURFACE_1_PATH "tests/surface_test/surface_1.png"
#define SURFACE_1_W 100
#define SURFACE_1_H 100
#define SURFACE_1_SIZE (SURFACE_1_W * SURFACE_1_H * sizeof(pixel_t))
#define SURFACE_1_SRCRECT NULL
#define SURFACE_1_DSTRECT &(const rect_t) { \
    (WINDOW_RECT.w - SURFACE_1_W) / 2,      \
    (WINDOW_RECT.h - SURFACE_1_H) / 2,      \
    SURFACE_1_W,                            \
    SURFACE_1_H                             \
}

static struct p_window *win = NULL;
static struct r_ctx *rctx = NULL;
static FILE *surface_1_fp = NULL;
static struct pixel_flat_data surface_1_data = { 0 };
static struct r_surface *surface_1 = NULL;

int main(void)
{
    s_configure_log(LOG_DEBUG, stdout, stderr);

    win = p_window_open(WINDOW_TITLE, &WINDOW_RECT, WINDOW_FLAGS);
    if (win == NULL)
        goto_error("Failed to open the window. Stop.");

    rctx = r_ctx_init(win, R_TYPE_SOFTWARE, 0);
    if (rctx == NULL)
        goto_error("Failed to initialize the renderer. Stop.");

    surface_1_fp = asset_fopen(SURFACE_1_PATH, "rb");
    if (surface_1_fp == NULL)
        goto_error("Failed to open surface 1 file (\"%s\"): %s. Stop.",
            SURFACE_1_PATH, strerror(errno));

    if (asset_load_plugin_by_type(IMG_TYPE_PNG))
        goto_error("Failed to load PNG plugin. Stop.");

    if (read_PNG(&surface_1_data, surface_1_fp))
        goto_error("Failed to read PNG data to surface pixels. Stop.");

    surface_1 = r_surface_init(rctx, &surface_1_data, P_WINDOW_BGRA8888);
    if (surface_1 == NULL)
        goto_error("Failed to create surface 1. Stop.");

    memset(&surface_1_data, 0, sizeof(struct pixel_flat_data));

    bool running = true;
    struct p_event ev = { 0 };
    while (running) {
        while (p_event_poll(&ev)) {
            if (ev.type == P_EVENT_QUIT)
                running = false;
        }
        r_reset(rctx);
        r_surface_blit(surface_1, SURFACE_1_SRCRECT, SURFACE_1_DSTRECT);
        r_surface_blit(surface_1, SURFACE_1_SRCRECT, &(rect_t) { 100, 100, 300, 100 });
        r_ctx_set_color(rctx, (color_RGBA32_t) { 255, 0, 255, 255 });
        r_draw_line(rctx, (vec2d_t) { 100, 100 }, (vec2d_t) { 110, 200 });
        r_flush(rctx);
        p_time_usleep(1000000 / FPS);
    }

    asset_unload_all_plugins();
    r_surface_destroy(surface_1);
    fclose(surface_1_fp);
    r_ctx_destroy(rctx);
    p_window_close(win);

    s_log_info("Test result is OK");
    return EXIT_SUCCESS;

err:
    if (surface_1_data.buf != NULL) free(surface_1_data.buf);

    asset_unload_all_plugins();
    if (surface_1 != NULL) r_surface_destroy(surface_1);
    if (surface_1_fp != NULL) fclose(surface_1_fp);
    if (rctx != NULL) r_ctx_destroy(rctx);
    if (win != NULL) p_window_close(win);
    s_log_info("Test result is FAIL");
    return EXIT_FAILURE;
}
