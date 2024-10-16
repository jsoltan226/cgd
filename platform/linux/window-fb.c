#include "../window.h"
#include <core/log.h>
#include <core/math.h>
#include <core/util.h>
#include <core/pixel.h>
#include <core/shapes.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <linux/mman.h>

#define P_INTERNAL_GUARD__
#include "window-fb.h"
#undef P_INTERNAL_GUARD__

#define MODULE_NAME "window-fb"

#define FBDEV_PATH "/dev/fb0"
#define FBDEV_FALLBACK_PATH "/dev/graphics/fb0"

i32 window_fb_open(struct window_fb *fb, const rect_t *area, const u32 flags)
{
    fb->closed = false;

    fb->fd = open(FBDEV_PATH, O_RDWR);
    if (fb->fd == -1) {
        s_log_warn("Failed to open framebuffer device '%s': %s",
            FBDEV_PATH, strerror(errno));

        fb->fd = open(FBDEV_FALLBACK_PATH, O_RDWR);
        if (fb->fd == -1)
            goto_error("Failed to open fallback framebuffer device '%s': %s",
                FBDEV_FALLBACK_PATH, strerror(errno));
    }

    /* Make sure the display is on */
    if (ioctl(fb->fd, FBIOBLANK, FB_BLANK_UNBLANK))
        goto_error("ioctl() failed: %s, make sure the display is on!", strerror(errno));

    /* Read fixed screen info */
    if (ioctl(fb->fd, FBIOGET_FSCREENINFO, &fb->fixed_info))
        goto_error("Failed to get fixed screen info: %s", strerror(errno));

    /* Read variable screen info */
    if (ioctl(fb->fd, FBIOGET_VSCREENINFO, &fb->var_info))
        goto_error("Failed to get variable screen info: %s", strerror(errno));

    if (fb->var_info.bits_per_pixel != 32)
        goto_error("Unsupported display bits per pixel '%i'", fb->var_info.bits_per_pixel);

    fb->mem_size = fb->fixed_info.smem_len;
    fb->mem = mmap(0, fb->mem_size, PROT_READ | PROT_WRITE, MAP_SHARED, fb->fd, 0);
    if (fb->mem == MAP_FAILED)
        goto_error("Failed to mmap() framebuffer device to program memory: %s",
            strerror(errno));

    fb->xres = fb->var_info.xres;
    fb->yres = fb->var_info.yres;
    fb->padding = (fb->fixed_info.line_length / sizeof(u32)) - fb->var_info.xres;


    memcpy(&fb->win_area, area, sizeof(rect_t));

    /* Handle P_WINDOW_POS_CENTERED flags */
    if (flags & P_WINDOW_POS_CENTERED_X)
        fb->win_area.x = abs((i32)fb->xres - area->w) / 2;

    if (flags & P_WINDOW_POS_CENTERED_Y)
        fb->win_area.y = abs((i32)fb->yres - area->h) / 2;

    fb->pixel_data.buf = NULL;
    fb->pixel_data.w = 0;
    fb->pixel_data.h = 0;

    s_log_debug("%s() OK; Screen is %ux%u, with %upx of padding", __func__,
        fb->xres, fb->yres, fb->padding);

    return 0;

err:
    window_fb_close(fb);
    return 1;
}

i32 window_fb_render_to_display(struct window_fb *fb)
{
    u_check_params(fb->pixel_data.buf != NULL);
    u32 x_offset = u_min(fb->win_area.x, fb->xres);
    u32 y_offset = u_min(fb->win_area.y, fb->yres);
    u32 w = u_min(u_min(fb->pixel_data.w, fb->win_area.w), fb->xres - x_offset);
    u32 h = u_min(u_min(fb->pixel_data.h, fb->win_area.h), fb->yres - y_offset);

    for (u32 y = 0; y < h; y++) {
        u32 dst_start_px_offset = (y + y_offset) * (fb->xres + fb->padding) + x_offset;
        u32 src_start_px_offset = y * fb->win_area.w;
        memcpy(
            fb->mem + dst_start_px_offset * sizeof(pixel_t), 
            fb->pixel_data.buf + src_start_px_offset,
            w * sizeof(pixel_t)
        );
    }
    return 0;
}

void window_fb_close(struct window_fb *fb)
{
    if (fb == NULL || fb->closed) return;

    s_log_debug("Closing framebuffer window...");
    if (fb->fd != -1) {
        close(fb->fd);
        fb->fd = -1;
    }
    if (fb->mem != NULL) {
        munmap(fb->mem, fb->mem_size);
        fb->mem = NULL;
        fb->mem_size = 0;
    }
    if (fb->pixel_data.buf != NULL) {
        free(fb->pixel_data.buf);
        fb->pixel_data.buf = NULL;
        fb->pixel_data.w = 0;
        fb->pixel_data.h = 0;
    }

    /* fb is supposed to be allocated on the stack, so no free() */
    fb->closed = true;

    /* All members (that need resetting) are already reset properly */
}

void window_fb_bind_fb(struct window_fb *win, struct pixel_flat_data *fb)
{
    if (win->pixel_data.buf != NULL) {
        s_log_error("A framebuffer is already bound to this window.");
        return;
    }
    win->pixel_data.buf = fb->buf;
    win->pixel_data.w = fb->w;
    win->pixel_data.h = fb->h;
}

void window_fb_unbind_fb(struct window_fb *win)
{
    if (win->pixel_data.buf != NULL) {
        free(win->pixel_data.buf);

        win->pixel_data.buf = NULL;
        win->pixel_data.w = 0;
        win->pixel_data.h = 0;
    }
}
