#include <core/log.h>
#include <core/pixel.h>
#include <core/shapes.h>
#include <asset-loader/img-type.h>
#include <asset-loader/plugin.h>
#include <asset-loader/asset.h>
#include <asset-loader/io-PNG.h>
#include <platform/window.h>
#include <render/rctx.h>
#include <render/rect.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define P_INTERNAL_GUARD__
#include <platform/linux/window-fb.h>
#undef P_INTERNAL_GUARD__

#define MODULE_NAME "fb-windowtest"

struct p_window {
    i32 type;

    i32 x, y, w, h;

    struct window_fb fb;
};

#define WINDOW_X 200
#define WINDOW_Y 200
#define WINDOW_W 500
#define WINDOW_H 500
#define WINDOW_FLAGS P_WINDOW_TYPE_FRAMEBUFFER

#define ASSET_REL_DIR   "tests/window_fb_test"

#define AMOGUS_IMG_FILEPATH         ASSET_REL_DIR "/amogus.png"
#define OUT_AMOGUS_IMG_FILEPATH     ASSET_REL_DIR "/amogus_out.png"
#define OUT_BOX_IMG_FILEPATH        ASSET_REL_DIR "/box.png"

#define BOX_X   500
#define BOX_Y   500
#define BOX_W   100
#define BOX_H   100
#define AMOGUS_X 800
#define AMOGUS_Y 100

static void pixel_data_RGBA32_to_BGRA32(struct pixel_flat_data *data);

static FILE *dev_null = NULL;
static void *prev_fb_mem = NULL;

int main(void)
{
    bool silent = getenv("SILENT") != NULL;

    if (silent) {
        dev_null = fopen("/dev/null", "wb");
        if (dev_null == NULL) {
            s_set_log_out_filep(stdout);
            s_set_log_err_filep(stderr);
            silent = false;
        }
        s_set_log_out_filep(dev_null);
        s_set_log_err_filep(dev_null);
    } else {
        s_set_log_out_filep(stdout);
        s_set_log_err_filep(stderr);
    }
    s_set_log_level(LOG_DEBUG);
    s_log_info("test");

    struct p_window *win = p_window_open(
        (const unsigned char *)MODULE_NAME,
        &(rect_t) { WINDOW_X, WINDOW_Y, WINDOW_W, WINDOW_H },
        WINDOW_FLAGS
    );
    if (win == NULL) {
        s_log_error("Failed to open window. Stop.");
        return EXIT_FAILURE;
    }
    
    if (silent) {
        prev_fb_mem = malloc(win->fb.mem_size);
        s_assert(prev_fb_mem != NULL, "malloc() failed for prev_fb_mem");
        memcpy(prev_fb_mem, win->fb.mem, win->fb.mem_size);
    }

    struct r_ctx *ctx = r_ctx_init(win, R_TYPE_SOFTWARE, 0);
    if (ctx == NULL) {
        s_log_error("Failed to initialize renderer context. Stop.");
        if (silent) {
            memcpy(win->fb.mem, prev_fb_mem, win->fb.mem_size);
            free(prev_fb_mem);
        }
        p_window_close(&win);
        return EXIT_FAILURE;
    }

    r_ctx_set_color(ctx, (color_RGBA32_t) { 128, 128, 0, 255 });
    r_fill_rect(ctx, 100, 100, 600, 300);
    r_ctx_set_color(ctx, (color_RGBA32_t) { 0, 255, 128, 255 });
    r_draw_rect(ctx, -50, -50, 400, 400);

    /* window border */
    r_ctx_set_color(ctx, WHITE_PIXEL);
    r_draw_rect(ctx, 0, 0, WINDOW_W - 1, WINDOW_H - 1);

    r_flush(ctx);
    sleep(2);
    
    r_ctx_destroy(&ctx);

    if (asset_load_plugin_by_type(IMG_TYPE_PNG)) {
        s_log_error("Failed to load libPNG. Skipping image test.");
        goto skip_img_test;
    }

    FILE *amogus_fp = asset_fopen(AMOGUS_IMG_FILEPATH, "rb");
    if (amogus_fp == NULL) {
        s_log_error("Failed to open asset \"%s\". Skipping image test.",
            AMOGUS_IMG_FILEPATH);
        goto skip_img_test;
    }

    struct pixel_flat_data amogus_pixel_data;
    if (read_PNG(&amogus_pixel_data, amogus_fp)) {
        s_log_error("Failed to read PNG image from \"%s\". Skipping image test.",
            AMOGUS_IMG_FILEPATH);
        goto skip_img_test;
    }

    fclose(amogus_fp);

    pixel_data_RGBA32_to_BGRA32(&amogus_pixel_data);

    win->fb.win_area.x = AMOGUS_X;
    win->fb.win_area.y = AMOGUS_Y;
    win->fb.win_area.w = amogus_pixel_data.w;
    win->fb.win_area.h = amogus_pixel_data.h;
    win->x = AMOGUS_X;
    win->y = AMOGUS_Y;
    win->w = amogus_pixel_data.w;
    win->h = amogus_pixel_data.h;

    p_window_bind_fb(win, &amogus_pixel_data);
    window_fb_render_to_display(&win->fb);
    p_window_unbind_fb(win);

    sleep(2);
skip_img_test:

    if (silent) {
        memcpy(win->fb.mem, prev_fb_mem, win->fb.mem_size);
        free(prev_fb_mem);
    }

    p_window_close(&win);
    asset_unload_all_plugins();

    if (dev_null != NULL) fclose(dev_null);

    return EXIT_SUCCESS;
}

static void pixel_data_RGBA32_to_BGRA32(struct pixel_flat_data *data)
{
    u32 tmp;
    for (u32 y = 0; y < data->h; y++) {
        for (u32 x = 0; x < data->w; x++) {
            pixel_t *const pixel_p = &data->buf[y * data->w + x];
            tmp = pixel_p->r;
            pixel_p->r = pixel_p->b;
            pixel_p->b = tmp;
        }
    }
}
