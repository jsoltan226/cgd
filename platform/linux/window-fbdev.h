#ifndef P_WINDOW_FBDEV_H_
#define P_WINDOW_FBDEV_H_

#ifndef P_INTERNAL_GUARD__
#error This header file is internal to the cgd platform module and is not intended to be used elsewhere
#endif /* P_INTERNAL_GUARD__ */

#include <core/int.h>
#include <core/pixel.h>
#include <core/shapes.h>
#include <stdbool.h>
#include <termios.h>
#include <linux/fb.h>

struct window_fbdev {
    i32 fd;
    struct fb_fix_screeninfo fixed_info;
    struct fb_var_screeninfo var_info;

    void *mem;
    u64 mem_size;

    u32 xres, yres;
    u32 padding;

    rect_t win_rect;
    rect_t display_rect;

    bool closed;

    i32 tty_fd;
    struct termios orig_term_config;
};

i32 window_fbdev_open(struct window_fbdev *win,
    const rect_t *area, const u32 flags);
void window_fbdev_close(struct window_fbdev *win);

void window_fbdev_render_to_display(struct window_fbdev *win,
    const struct pixel_flat_data *fb);

#endif /* P_WINDOW_FBDEV_H_ */
