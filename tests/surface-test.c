#include "core/pressable-obj.h"
#include "platform/keyboard.h"
#include <core/log.h>
#include <core/util.h>
#include <core/pixel.h>
#include <core/shapes.h>
#include <platform/time.h>
#include <platform/event.h>
#include <platform/window.h>
#include <render/line.h>
#include <render/rctx.h>
#include <render/rect.h>
#include <render/surface.h>
#include <asset-loader/asset.h>
#include <asset-loader/plugin.h>
#include <asset-loader/img-type.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define MODULE_NAME "surface-test"

#define WINDOW_TITLE (const unsigned char *)MODULE_NAME
#define WINDOW_RECT (const rect_t) { 0, 0, 500, 500 }
#define WINDOW_FLAGS (P_WINDOW_TYPE_AUTO | P_WINDOW_POS_CENTERED_XY)

#define FPS 60

#define SURFACE_W 100
#define SURFACE_H 100
#define SURFACE_SIZE (SURFACE_1_W * SURFACE_1_H * sizeof(pixel_t))
#define SURFACE_SRCRECT NULL

#define SURFACE_DSTRECT_1 (const rect_t) { \
    (WINDOW_RECT.w - SURFACE_W) / 2,        \
    (WINDOW_RECT.h - SURFACE_H) / 2,        \
    SURFACE_W,                              \
    SURFACE_H                               \
}
#define SURFACE_DSTRECT_2 (const rect_t) { 100, 100, 300, 100 }
#define SURFACE_DSTRECT_3 (const rect_t) { 10, 0, 300, 1000 }
#define SURFACE_DSTRECT_4 (const rect_t) { 100, 100, 50, 50 }
#define SURFACE_DSTRECT_5 (const rect_t) { 100, 100, 0, 100 }
#define SURFACE_DSTRECT_6 (const rect_t) { 100, 100, 100, -30 }

static struct p_window *win = NULL;
static struct p_keyboard *kb = NULL;
static struct r_ctx *rctx = NULL;
static struct asset *asset = NULL;

int main(void)
{
    s_configure_log(LOG_DEBUG, stdout, stderr);

    win = p_window_open(WINDOW_TITLE, &WINDOW_RECT, WINDOW_FLAGS);
    if (win == NULL)
        goto_error("Failed to open the window. Stop.");

    kb = p_keyboard_init(win);
    if (kb == NULL)
        goto_error("Failed to initialize the keyboard. Stop.");

    rctx = r_ctx_init(win, R_TYPE_SOFTWARE, 0);
    if (rctx == NULL)
        goto_error("Failed to initialize the renderer. Stop.");

    static const filepath_t asset_filepath = "tests/surface_test/surface_1.png";
    asset = asset_load(asset_filepath, rctx);
    if (asset == NULL)
        goto_error("Failed to load surface image (\"%s\"): %s. Stop.",
            asset_filepath);

    bool running = true;
    struct p_event ev = { 0 };

    f32 scale = 1.f;
    const pressable_obj_t *up_key =
        p_keyboard_get_key(kb, KB_KEYCODE_ARROWUP);
    const pressable_obj_t *down_key =
        p_keyboard_get_key(kb, KB_KEYCODE_ARROWDOWN);
    while (running) {
        while (p_event_poll(&ev)) {
            if (ev.type == P_EVENT_QUIT)
                running = false;
        }
        r_reset(rctx);
        r_ctx_set_color(rctx, (color_RGBA32_t) { 0, 255, 0, 255 });

        p_keyboard_update(kb);
        if (up_key->down) scale += 0.5;
        if (down_key->down) scale -= 0.5;

        /* Should display the image in 1:1 scale with the actual src,
         * right in the middle of the window */ 
        //r_surface_blit(asset->surface, SURFACE_SRCRECT, &SURFACE_DSTRECT_1);
        //r_draw_rect(rctx, rect_arg_expand(SURFACE_DSTRECT_1));

        /* Should display the image right in the middle of the window */
        //r_surface_blit(asset->surface, SURFACE_SRCRECT, &SURFACE_DSTRECT_2);
        //r_draw_rect(rctx, rect_arg_expand(SURFACE_DSTRECT_2));

        /* Should display a highly vertically-streched image */
        //r_surface_blit(asset->surface, SURFACE_SRCRECT, &SURFACE_DSTRECT_3);
        //r_draw_rect(rctx, rect_arg_expand(SURFACE_DSTRECT_3));

        /* Should only display the right part of the image;
         * the rest should be cut off by the window border */
        rect_t dst_rect = { rect_arg_expand(SURFACE_DSTRECT_4) };
        dst_rect.w *= scale;
        dst_rect.h *= scale;
        dst_rect.x -= (dst_rect.w - SURFACE_DSTRECT_4.w) / 2;
        dst_rect.y -= (dst_rect.h - SURFACE_DSTRECT_4.h) / 2;
        r_surface_blit(asset->surface, SURFACE_SRCRECT, &dst_rect);
        r_draw_rect(rctx, rect_arg_expand(dst_rect));
 
        /* Should display nothing; the dst width is 0 */
        r_surface_blit(asset->surface, SURFACE_SRCRECT, &SURFACE_DSTRECT_5);
        /* Should display nothing; the dst height is negative */
        r_surface_blit(asset->surface, SURFACE_SRCRECT, &SURFACE_DSTRECT_6);

        r_flush(rctx);
        p_time_usleep(1000000 / FPS);
    }

    asset_destroy(&asset);
    asset_unload_all_plugins();
    r_ctx_destroy(&rctx);
    p_keyboard_destroy(&kb);
    p_window_close(&win);

    s_log_info("Test result is OK");
    return EXIT_SUCCESS;

err:
    if (asset != NULL) asset_destroy(&asset);
    asset_unload_all_plugins();
    if (rctx != NULL) r_ctx_destroy(&rctx);
    if(kb != NULL) p_keyboard_destroy(&kb);
    if (win != NULL) p_window_close(&win);
    s_log_info("Test result is FAIL");
    return EXIT_FAILURE;
}
