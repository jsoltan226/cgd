#ifndef P_WINDOW_FBDEV_H_
#define P_WINDOW_FBDEV_H_

#ifndef P_INTERNAL_GUARD__
#error This header file is internal to the cgd platform module and is not intended to be used elsewhere
#endif /* P_INTERNAL_GUARD__ */

#include "../window.h"
#include <core/int.h>
#include <core/pixel.h>
#include <core/shapes.h>
#include <stdbool.h>
#include <termios.h>
#include <pthread.h>
#include <semaphore.h>
#include <linux/fb.h>

struct window_fbdev_listener {
    _Atomic bool running;

    pthread_t thread;
    pthread_mutex_t buf_mutex;

    sem_t page_flip_pending;
    /* The thread only needs to read from the buffer
     * and write to the map */
    const struct pixel_flat_data *_Atomic front_buffer_p;
    void *const *map_p;

    /* All of these should be read-only for the thread */
    const i32 *fd_p;
    const rect_t *win_rect_p, *display_rect_p;
    const u32 *stride_p;
};

struct window_fbdev {
    i32 fd;
    struct fb_fix_screeninfo fixed_info;
    struct fb_var_screeninfo var_info;

    void *mem;
    u64 mem_size;

    struct pixel_flat_data back_buffer;
    struct pixel_flat_data front_buffer;

    struct window_fbdev_listener listener;

    u32 xres, yres;
    u32 padding;
    u32 stride;
    u32 refresh_rate;

    rect_t win_rect;
    rect_t display_rect;

    bool closed;

    i32 tty_fd;
    struct termios orig_term_config;
};

i32 window_fbdev_open(struct window_fbdev *win,
    const rect_t *area, const u32 flags);
void window_fbdev_close(struct window_fbdev *win);

struct pixel_flat_data * window_fbdev_swap_buffers(struct window_fbdev *win,
    const enum p_window_present_mode present_mode);

#endif /* P_WINDOW_FBDEV_H_ */
