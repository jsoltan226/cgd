#include "core/pixel.h"
#include "core/shapes.h"
#include <linux/mman.h>
#include <unistd.h>
#include "core/util.h"
#include "core/log.h"
#include "core/math.h"
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/fb.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>

#define P_INTERNAL_GUARD__
#include "window-fb.h"
#undef P_INTERNAL_GUARD__

#define MODULE_NAME "window"

#define FBDEV_PATH "/dev/fb0"

i32 window_fb_open(struct window_fb *fb, const rect_t *area)
{
    fb->fd = open(FBDEV_PATH, O_RDWR);
    if (fb->fd == -1)
        goto_error("Failed to open framebuffer device '%s': %s",
            FBDEV_PATH, strerror(errno));

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

    s_log_debug("%s() OK; Screen is %ux%u, with %upx of padding", __func__,
        fb->xres, fb->yres, fb->padding);

    return 0;

err:
    window_fb_close(fb);
    return 1;
}

i32 window_fb_render_to_display(struct window_fb *fb,
    const pixel_t *buf, const rect_t *frame)
{
    if (fb == NULL || buf == NULL || frame == NULL) {
        s_log_error("invalid parameters");
        return 1;
    }

    u32 x_offset = min(frame->x, fb->xres);
    u32 y_offset = min(frame->y, fb->yres);
    u32 w = min(frame->w, fb->xres - x_offset);
    u32 h = min(frame->h, fb->yres - y_offset);

    for (u32 y = 0; y < h; y++) {
        u32 dst_start_px_offset = (y + y_offset) * (fb->xres + fb->padding) + x_offset;
        u32 src_start_px_offset = y * frame->w;
        memcpy(
            fb->mem + dst_start_px_offset * sizeof(pixel_t), 
            buf + src_start_px_offset,
            w * sizeof(pixel_t)
        );
    }
    return 0;
}

void window_fb_close(struct window_fb *fb)
{
    if (fb == NULL) return;

    s_log_debug("Closing framebuffer window...");
    close(fb->fd);
    munmap(fb->mem, fb->mem_size);
    /* fb is supposed to be allocated on the stack, so no free() */
}
