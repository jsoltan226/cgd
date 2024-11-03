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
#include <termios.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <linux/mman.h>

#define P_INTERNAL_GUARD__
#include "window-fbdev.h"
#undef P_INTERNAL_GUARD__
#define P_INTERNAL_GUARD__
#include "keyboard-tty.h"
#undef P_INTERNAL_GUARD__

#define MODULE_NAME "window-fbdev"

#define FBDEV_PATH "/dev/fb0"
#define FBDEV_FALLBACK_PATH "/dev/graphics/fb0"

i32 window_fbdev_open(struct window_fbdev *win,
    const rect_t *area, const u32 flags)
{
    win->closed = false;
    win->tty_fd = -1;

    win->fd = open(FBDEV_PATH, O_RDWR);
    if (win->fd == -1) {
        s_log_warn("Failed to open framebuffer device '%s': %s",
            FBDEV_PATH, strerror(errno));

        win->fd = open(FBDEV_FALLBACK_PATH, O_RDWR);
        if (win->fd == -1)
            goto_error("Failed to open fallback framebuffer device '%s': %s",
                FBDEV_FALLBACK_PATH, strerror(errno));
    }

    /* Make sure the display is on */
    if (ioctl(win->fd, FBIOBLANK, FB_BLANK_UNBLANK))
        goto_error("ioctl() failed: %s, make sure the display is on!", strerror(errno));

    /* Read fixed screen info */
    if (ioctl(win->fd, FBIOGET_FSCREENINFO, &win->fixed_info))
        goto_error("Failed to get fixed screen info: %s", strerror(errno));

    /* Read variable screen info */
    if (ioctl(win->fd, FBIOGET_VSCREENINFO, &win->var_info))
        goto_error("Failed to get variable screen info: %s", strerror(errno));

    if (win->var_info.bits_per_pixel != 32)
        goto_error("Unsupported display bits per pixel '%i'", win->var_info.bits_per_pixel);

    win->mem_size = win->fixed_info.smem_len;
    win->mem = mmap(0, win->mem_size, PROT_READ | PROT_WRITE, MAP_SHARED, win->fd, 0);
    if (win->mem == MAP_FAILED)
        goto_error("Failed to mmap() framebuffer device to program memory: %s",
            strerror(errno));

    win->xres = win->var_info.xres;
    win->yres = win->var_info.yres;
    win->padding = (win->fixed_info.line_length / sizeof(u32)) - win->var_info.xres;

    win->display_rect.x = 0;
    win->display_rect.y = 0;
    win->display_rect.w = win->xres;
    win->display_rect.h = win->yres;

    memcpy(&win->win_rect, area, sizeof(rect_t));
    /* Handle P_WINDOW_POS_CENTERED flags */
    if (flags & P_WINDOW_POS_CENTERED_X)
        win->win_rect.x = abs((i32)win->xres - area->w) / 2;

    if (flags & P_WINDOW_POS_CENTERED_Y)
        win->win_rect.y = abs((i32)win->yres - area->h) / 2;

    /* Set the terminal to raw mode to avoid echoing user input
     * on the console */
    win->tty_fd = open(TTYDEV_FILEPATH, O_RDWR | O_NONBLOCK);
    if (win->tty_fd == -1) goto skip_tty_raw_mode;

    (void) tty_keyboard_set_term_raw_mode(win->tty_fd,
        &win->orig_term_config, NULL);
skip_tty_raw_mode:

    s_log_debug("%s() OK; Screen is %ux%u, with %upx of padding", __func__,
        win->xres, win->yres, win->padding);

    return 0;

err:
    window_fbdev_close(win);
    return 1;
}

void window_fbdev_render_to_display(struct window_fbdev *win,
    const struct pixel_flat_data *fb)
{
    if (fb == NULL || fb->buf == NULL || win->mem == NULL) return;

    rect_t dst = win->win_rect;
    rect_clip(&dst, &win->display_rect);
    /* "Project" the changes made to dst onto src */
    rect_t src = {
        .x = -(win->win_rect.x - dst.x),
        .y = -(win->win_rect.y - dst.y),
        .w = fb->w - (win->win_rect.w - dst.w),
        .h = fb->h - (win->win_rect.h - dst.h),
    };

    for (u32 y = 0; y < src.h; y++) {
        const u32 dst_offset = (
            (dst.y + y) * (win->xres + win->padding)
            + dst.x
        ) * sizeof(pixel_t);
        const u32 src_offset = (
            ((src.y + y) * src.w)
            + src.x 
        );

        memcpy(
            win->mem + dst_offset, 
            fb->buf + src_offset,
            src.w * sizeof(pixel_t)
        );
    }
}

void window_fbdev_close(struct window_fbdev *win)
{
    if (win == NULL || win->closed) return;

    s_log_debug("Closing framebuffer window...");
    if (win->fd != -1) {
        close(win->fd);
        win->fd = -1;
    }
    if (win->mem != NULL) {
        munmap(win->mem, win->mem_size);
        win->mem = NULL;
        win->mem_size = 0;
    }

    if (win->tty_fd != -1) {
        /* Restore the terminal configuration */
        (void) tcsetattr(win->tty_fd, TCSANOW, &win->orig_term_config);

        /* Discard any characters written on stdin
         * (Clear the command line) */
        (void) tcflush(win->tty_fd, TCIOFLUSH);

        /* Close the tty file descriptor */
        close(win->tty_fd);
        win->tty_fd = -1;
    }

    /* fb is supposed to be allocated on the stack, so no free() */
    win->closed = true;

    /* All members (that need resetting) are already reset properly */
}
