#define _GNU_SOURCE
#include "../ptime.h"
#include "../event.h"
#include "../window.h"
#include <core/log.h>
#include <core/math.h>
#include <core/util.h>
#include <core/pixel.h>
#include <core/shapes.h>
#include <errno.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
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

static void * window_fbdev_listener_fn(void *arg);
static void empty_handler(i32 sig_num);
static void write_to_fb(void *map, const u32 stride, const rect_t *display_rect,
    const rect_t *win_rect, const struct pixel_flat_data *pixels);

i32 window_fbdev_open(struct window_fbdev *win,
    const rect_t *area, const u32 flags)
{
    if (flags & P_WINDOW_REQUIRE_ACCELERATED)
        goto_error("GPU acceleration requried, "
            "but fbdev windows don't support it.");

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
    win->stride = win->xres + win->padding;

    s_log_debug("win->var_info.xres_virtual: %u, win->var_info.yres_virtual: %u",
        win->var_info.xres_virtual, win->var_info.yres_virtual);

    /* Calculate the refresh rate */
    const u64 total_px_horizontal = win->var_info.xres + win->var_info.hsync_len
        + win->var_info.left_margin + win->var_info.right_margin;
    const u64 total_px_vertical = win->var_info.yres + win->var_info.vsync_len
        + win->var_info.upper_margin + win->var_info.lower_margin;
    const u64 total_px = total_px_vertical * total_px_horizontal;

    /* 10^12 / pixclock */
    if (win->var_info.pixclock == 0) {
        s_log_warn("The pixel clock is set to 0; "
            "monitor refresh rate cannot be calculated.");
        win->refresh_rate = 0;
    } else {
        const f64 pixel_clock_hz = 1000000000000.F / win->var_info.pixclock;
        s_assert(total_px != 0, "Division by zero");
        win->refresh_rate = (u32)(pixel_clock_hz / total_px);
    }

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
    if (win->tty_fd != -1) {
        (void) tty_keyboard_set_term_raw_mode(win->tty_fd,
            &win->orig_term_config, NULL);
    }

    /* Test the vsync ioctl */
    i32 dummy = 0;
    if (ioctl(win->fd, FBIO_WAITFORVSYNC, &dummy))
        goto_error("Failed to wait for vsync: %s", strerror(errno));

    /* Allocate the buffers */
    win->back_buffer.w = win->listener.front_buffer.w = win->xres;
    win->back_buffer.h = win->listener.front_buffer.h = win->yres;
    const u64 n_pixels = win->xres * win->yres;
    win->back_buffer.buf = calloc(n_pixels, sizeof(pixel_t));
    win->listener.front_buffer.buf = calloc(n_pixels, sizeof(pixel_t));
    if (win->back_buffer.buf == NULL || win->listener.front_buffer.buf == NULL)
        goto_error("Failed to allocate the pixel buffers");

    /* Init the listener thread */
    win->listener.fd_p = &win->fd;
    win->listener.win_rect_p = &win->win_rect;
    win->listener.display_rect_p = &win->display_rect;
    win->listener.map_p = &win->mem;
    win->listener.stride_p = &win->stride;
    atomic_store(&win->listener.page_flip_pending, false);

    /* Always returns 0 */
    (void) pthread_mutex_init(&win->listener.buf_mutex, NULL);
    atomic_store(&win->listener.running, true);

    i32 ret = pthread_create(&win->listener.thread, NULL,
            window_fbdev_listener_fn, &win->listener);
    if (ret != 0)
        goto_error("Failed to spawn the listener thread: %s", strerror(ret));

    s_log_debug("%s() OK; Screen is %ux%u@%uHz, with %upx of padding", __func__,
        win->xres, win->yres, win->refresh_rate, win->padding);

    return 0;

err:
    /* window_fbdev_close() will be called in p_window_close() */
    return 1;
}

void window_fbdev_close(struct window_fbdev *win)
{
    if (win == NULL || win->closed) return;

    s_log_debug("Closing framebuffer window...");

    /* Clean up the thread */
    if (atomic_load(&win->listener.running)) {
        /* Signal the thread and wait for it to terminate */
        atomic_store(&win->listener.running, false);
        (void) pthread_kill(win->listener.thread, SIGUSR1);

        struct timespec thread_timeout;

        if (clock_gettime(CLOCK_REALTIME, &thread_timeout)) {
            s_log_error("Failed to get the current time: %s", strerror(errno));
            goto kill_thread;
        }

        /* Set the timeout to 1 second from now */
        thread_timeout.tv_sec += 1;

        i32 ret = pthread_timedjoin_np(win->listener.thread,
            NULL, &thread_timeout);

        /* If it couldn't exit in time on it's own, help it */
        if (ret != 0) {
            if (ret == ETIMEDOUT) {
                s_log_error("Timed out while waiting "
                    "for the listener thread to terminate. ");
            } else {
                s_log_error("Failed to join the listener thread: %s",
                    strerror(ret));
            }

kill_thread:
            s_log_warn("Killing the listener thread...");
            (void) pthread_kill(win->listener.thread, SIGKILL);
        }
    }

    (void) pthread_mutex_destroy(&win->listener.buf_mutex);

    /* Free the pixel buffers */
    if (win->listener.front_buffer.buf != NULL)
        u_nfree(&win->listener.front_buffer.buf);
    if (win->back_buffer.buf != NULL)
        u_nfree(&win->back_buffer.buf);

    /* Reset the tty back to its normal state */
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

    /* Unmap and close the device */
    if (win->mem != NULL) {
        munmap(win->mem, win->mem_size);
        win->mem = NULL;
        win->mem_size = 0;
    }
    if (win->fd != -1) {
        close(win->fd);
        win->fd = -1;
    }

    /* fb is supposed to be allocated on the stack, so no free() */
    memset(win, 0, sizeof(struct window_fbdev));
    win->closed = true;
}

void window_fbdev_render(struct window_fbdev *win,
    const struct pixel_flat_data *pixels)
{
}

struct pixel_flat_data * window_fbdev_swap_buffers(struct window_fbdev *win)
{
    if (win == NULL) return NULL;

    /* If a page flip is in progress, wait for it to finish */
    pthread_mutex_lock(&win->listener.buf_mutex);
    atomic_store(&win->listener.page_flip_pending, true);

    /* We can just swap the buffers themselves as the metadata stays the same */
    pixel_t *new_back_buffer = win->listener.front_buffer.buf;
    win->listener.front_buffer.buf = win->back_buffer.buf;
    win->back_buffer.buf = new_back_buffer;
    pthread_mutex_unlock(&win->listener.buf_mutex);

    return &win->back_buffer;
}

static void write_to_fb(void *map, const u32 stride, const rect_t *display_rect,
    const rect_t *win_rect, const struct pixel_flat_data *pixels)
{
    if (map == NULL || display_rect == NULL || win_rect == NULL
        || pixels == NULL || pixels->buf == NULL) return;

    rect_t area;
    memcpy(&area, win_rect, sizeof(rect_t));
    rect_clip(&area, display_rect);

    for (u32 y = 0; y < area.h; y++) {
        const u32 dst_offset = (
            ((area.y + y) * stride)
            + area.x
        ) * sizeof(pixel_t);
        const u32 src_offset = (
            ((area.y + y) * pixels->w)
            + area.x
        );

        memcpy(
            map + dst_offset,
            pixels->buf + src_offset,
            area.w * sizeof(pixel_t)
        );
    }
}

static void * window_fbdev_listener_fn(void *arg)
{
    struct window_fbdev_listener *listener = arg;

    /* Sanity checks */
    s_assert(listener->fd_p != NULL && *listener->fd_p != -1,
        "The fbdev file descriptor is not initialized!");
    s_assert(listener->win_rect_p != NULL,
        "The window rect pointer is not initialized!");
    s_assert(listener->display_rect_p != NULL,
        "The display rect pointer is not initialized!");
    s_assert(listener->map_p != NULL && *listener->map_p != NULL,
        "The framebuffer memory map is not initialized!");
    s_assert(listener->stride_p != NULL,
        "The stride pointer is not initialized!");

    /* Only allow SIGUSR1 */
    sigset_t sig_mask;
    sigfillset(&sig_mask);
    sigdelset(&sig_mask, SIGUSR1);
    i32 ret = pthread_sigmask(SIG_SETMASK, &sig_mask, NULL);
    s_assert(ret == 0, "Failed to set the listener thread's signal mask: %s",
        strerror(ret));

    /* Set the signal handler for SIGUSR1 */
    struct sigaction sa = { 0 };
    sa.sa_handler = empty_handler;
    sa.sa_flags = 0;
    /* Block all other signals while the handler is running */
    sigfillset(&sa.sa_mask);

    /* Should only fail with EINVAL (invalid argument) */
    s_assert(sigaction(SIGUSR1, &sa, NULL) == 0,
        "Failed to set the SIGUSR1 handler: %s", strerror(errno));

    while (atomic_load(&listener->running)) {
        if (atomic_load(&listener->page_flip_pending)) {
            pthread_mutex_lock(&listener->buf_mutex);
            timestamp_t start;
            p_time_get_ticks(&start);
            write_to_fb(*listener->map_p, *listener->stride_p,
                listener->display_rect_p, listener->win_rect_p,
                &listener->front_buffer);
            atomic_store(&listener->page_flip_pending, false);
            s_log_debug("memcpy took %lu micro-seconds", p_time_delta_us(&start));
            pthread_mutex_unlock(&listener->buf_mutex);

            /* Inform everyone that a page flip has occured */
            const struct p_event ev = {
                .type = P_EVENT_PAGE_FLIP,
                .info.page_flip_status = 0,
            };
            p_event_send(&ev);

            i32 dummy = 0;
            if (ioctl(*listener->fd_p, FBIO_WAITFORVSYNC, &dummy)) {
                if (errno == EINTR) /* Interrupted by signal */
                    continue;
                else
                    s_log_error("Failed to wait for vsync: %s", strerror(errno));
            }
        }
    }

    pthread_exit(NULL);
}

static void empty_handler(i32 sig_num)
{
    (void) sig_num;
}
