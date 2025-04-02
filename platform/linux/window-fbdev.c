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
#include <semaphore.h>
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
static i32 post_sem_if_blocked(sem_t *sem);

i32 window_fbdev_open(struct window_fbdev *win,
    const rect_t *area, const u32 flags,
    struct p_window_info *info)
{
    memset(win, 0, sizeof(struct window_fbdev));
    win->closed = false;
    win->tty_fd = -1;
    win->generic_info_p = info;

    if (flags & P_WINDOW_REQUIRE_ACCELERATED ||
        flags & P_WINDOW_REQUIRE_OPENGL ||
        flags & P_WINDOW_REQUIRE_VULKAN)
        goto_error("GPU acceleration requried, "
            "but fbdev windows don't support it.");
    else if (flags & P_WINDOW_PREFER_ACCELERATED)
        s_log_warn("Acceleration not supported for fbdev windows.");

    win->fd = open(FBDEV_PATH, O_RDWR | O_CLOEXEC);
    if (win->fd == -1 && errno == ENOENT) { /* No such file or directory */
        win->fd = open(FBDEV_FALLBACK_PATH, O_RDWR | O_CLOEXEC);
        if (win->fd == -1)
            goto_error("Failed to open fallback framebuffer device '%s': %s",
                FBDEV_FALLBACK_PATH, strerror(errno));
    } else if (win->fd == -1) {
        goto_error("Failed to open framebuffer device '%s': %s",
            FBDEV_PATH, strerror(errno));
    }

    /* Make sure the display is on */
    if (ioctl(win->fd, FBIOBLANK, FB_BLANK_UNBLANK))
        goto_error("FBIOBLANK failed: %s, make sure the display is on!",
            strerror(errno));

    /* Read fixed screen info */
    if (ioctl(win->fd, FBIOGET_FSCREENINFO, &win->fixed_info))
        goto_error("Failed to get fixed screen info: %s", strerror(errno));

    /* Read variable screen info */
    if (ioctl(win->fd, FBIOGET_VSCREENINFO, &win->var_info))
        goto_error("Failed to get variable screen info: %s", strerror(errno));

    if (win->var_info.bits_per_pixel != 32)
        goto_error("Unsupported display bits per pixel '%i'",
            win->var_info.bits_per_pixel);

    info->display_color_format = BGRX32;

    win->mem_size = win->fixed_info.smem_len;
    win->mem = mmap(0, win->mem_size, PROT_READ | PROT_WRITE,
        MAP_SHARED, win->fd, 0);
    if (win->mem == MAP_FAILED)
        goto_error("Failed to mmap() framebuffer device to program memory: %s",
            strerror(errno));

    win->xres = win->var_info.xres;
    win->yres = win->var_info.yres;
    win->padding = (win->fixed_info.line_length / sizeof(u32))
        - win->var_info.xres;
    win->stride = win->xres + win->padding;

    /* Calculate the refresh rate */
    const u64 total_px_horizontal = win->var_info.xres + win->var_info.hsync_len
        + win->var_info.left_margin + win->var_info.right_margin;
    const u64 total_px_vertical = win->var_info.yres + win->var_info.vsync_len
        + win->var_info.upper_margin + win->var_info.lower_margin;
    const u64 total_px = total_px_vertical * total_px_horizontal;

    /* 10^12 / pixclock */
    if (win->var_info.pixclock == 0) {
        s_log_debug("The pixel clock is set to 0; "
            "monitor refresh rate cannot be calculated.");
        win->refresh_rate = 0;
    } else {
        const f64 pixel_clock_hz = 1000000000000.F / win->var_info.pixclock;
        s_assert(total_px != 0, "Division by zero");
        win->refresh_rate = (u32)(pixel_clock_hz / total_px);
    }

    info->display_rect.x = 0;
    info->display_rect.y = 0;
    info->display_rect.w = win->xres;
    info->display_rect.h = win->yres;

    /* Handle P_WINDOW_POS_CENTERED flags */
    if (flags & P_WINDOW_POS_CENTERED_X)
        info->client_area.x = abs((i32)win->xres - (i32)area->w) / 2;

    if (flags & P_WINDOW_POS_CENTERED_Y)
        info->client_area.y = abs((i32)win->yres - (i32)area->h) / 2;

    /* Set the terminal to raw mode to avoid echoing user input
     * on the console */
    win->tty_fd = open(TTYDEV_FILEPATH, O_RDWR | O_NONBLOCK | O_CLOEXEC);
    if (win->tty_fd != -1) {
        (void) tty_keyboard_set_term_raw_mode(win->tty_fd,
            &win->orig_term_config, NULL);
    }

    /* Test the vsync ioctl */
    i32 dummy = 0;
    if (ioctl(win->fd, FBIO_WAITFORVSYNC, &dummy))
        goto_error("Failed to wait for vsync: %s", strerror(errno));

    /* Allocate the buffers */
    win->back_buffer.w = win->front_buffer.w = area->w;
    win->back_buffer.h = win->front_buffer.h = area->h;
    const u64 n_pixels = win->xres * win->yres;
    win->back_buffer.buf = calloc(n_pixels, sizeof(pixel_t));
    win->front_buffer.buf = calloc(n_pixels, sizeof(pixel_t));
    if (win->back_buffer.buf == NULL || win->front_buffer.buf == NULL)
        goto_error("Failed to allocate the pixel buffers");

    /* Init the listener thread */
    if (sem_init(&win->listener.page_flip_pending, 0, 0))
        goto_error("Failed to init the page flip semaphore");
    atomic_store(&win->listener.front_buffer_p, NULL);
    win->listener.map_p = &win->mem;

    win->listener.fd_p = &win->fd;
    win->listener.win_rect_p = &win->generic_info_p->client_area;
    win->listener.display_rect_p = &win->generic_info_p->display_rect;
    win->listener.stride_p = &win->stride;

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
    (void) sem_destroy(&win->listener.page_flip_pending);

    /* Free the pixel buffers */
    if (win->front_buffer.buf != NULL)
        u_nfree(&win->front_buffer.buf);
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

/* Swaps the buffers and informs the listener thread to perform a page flip */
struct pixel_flat_data * window_fbdev_swap_buffers(struct window_fbdev *win,
    const enum p_window_present_mode present_mode)
{
    if (win == NULL) return NULL;

    /* If another page flip is in progress, wait for it to finish */
    pthread_mutex_lock(&win->listener.buf_mutex);

    /* We can just swap the pointers themselves since the metadata
     * (width and height) is the same for both buffers */
    pixel_t *new_back_buffer = win->front_buffer.buf;
    win->front_buffer.buf = win->back_buffer.buf;
    win->back_buffer.buf = new_back_buffer;

    /* If vsync is on, let the listener thread do the rendering
     * when it receives a vblank event.
     * Otherwise, just copy the front buffer to the screen. */
    switch (present_mode) {
    case P_WINDOW_PRESENT_VSYNC:
        atomic_store(&win->listener.front_buffer_p, &win->front_buffer);
        if (post_sem_if_blocked(&win->listener.page_flip_pending)) {
            s_log_error("Failed to post the page flip semaphore");
            pthread_mutex_unlock(&win->listener.buf_mutex);
            return NULL;
        }
        break;
    case P_WINDOW_PRESENT_NOW:
        write_to_fb(win->mem, win->stride,
            &win->generic_info_p->display_rect,
            &win->generic_info_p->client_area,
            &win->front_buffer);
        break;
    }

    pthread_mutex_unlock(&win->listener.buf_mutex);

    return &win->back_buffer;
}

static void write_to_fb(void *map, const u32 stride, const rect_t *display_rect,
    const rect_t *win_rect, const struct pixel_flat_data *pixels)
{
    if (map == NULL || display_rect == NULL || win_rect == NULL
        || pixels == NULL || pixels->buf == NULL) return;

    rect_t dst;
    memcpy(&dst, win_rect, sizeof(rect_t));
    rect_clip(&dst, display_rect);

    /* "Project" the changes made to dst onto src */
    const rect_t src = {
        .x = dst.x - win_rect->x,
        .y = dst.y - win_rect->y,
        .w = pixels->w + (dst.w - win_rect->w),
        .h = pixels->h + (dst.h - win_rect->h),
    };

    /* Fix pointer arithmetic shenanigans */
    u8 *src_mem = (u8 *)pixels->buf;
    u8 *dst_mem = (u8 *)map;
    const u32 src_stride_bytes = src.w * sizeof(pixel_t);

    /* Blit the window to the screen */
    for (u32 y = 0; y < src.h; y++) {
        const u32 dst_offset = (
            ((dst.y + y) * stride)
            + dst.x
        ) * sizeof(pixel_t);
        const u32 src_offset = (
            ((src.y + y) * src.w)
            + src.x
        ) * sizeof(pixel_t);

        memcpy(
            dst_mem + dst_offset,
            src_mem + src_offset,
            src_stride_bytes
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
        if (sem_wait(&listener->page_flip_pending)) {
            if (errno == EINTR) continue; /* Interrupted by signal */
            else {
                s_log_error("Failed to wait on the page flip semaphore: %s",
                    strerror(errno));
                break;
            }
        }
        pthread_mutex_lock(&listener->buf_mutex);

        /* Wait for VSync */
        i32 dummy = 0;
        if (ioctl(*listener->fd_p, FBIO_WAITFORVSYNC, &dummy)) {
            if (errno == EINTR) /* Interrupted by signal */
                continue;
            else
                s_log_error("Failed to wait for vsync: %s", strerror(errno));
        }

        write_to_fb(*listener->map_p, *listener->stride_p,
            listener->display_rect_p, listener->win_rect_p,
            atomic_load(&listener->front_buffer_p));

        atomic_store(&listener->front_buffer_p, NULL);
        pthread_mutex_unlock(&listener->buf_mutex);

        timestamp_t time;
        p_time(&time);

        /* Inform everyone that a page flip has occured */
        const struct p_event ev = {
            .type = P_EVENT_PAGE_FLIP,
            .info.page_flip_status = 0,
        };
        p_event_send(&ev);
    }

    pthread_exit(NULL);
}

static void empty_handler(i32 sig_num)
{
    (void) sig_num;
}

static i32 post_sem_if_blocked(sem_t *sem)
{
    i32 value = 0;
    if (sem_getvalue(sem, &value)) {
        s_log_error("Failed to get the value of a semaphore: %s",
            strerror(errno));
        return 1;
    }

    if (value == 0) {
        if (sem_post(sem)) {
            s_log_error("Failed to post a semaphore: %s", strerror(errno));
            return 1;
        }
    }

    return 0;
}
