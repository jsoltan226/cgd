#ifndef P_WINDOW_FB_H_
#define P_WINDOW_FB_H_

#ifndef P_INTERNAL_GUARD__
#error This header file is internal to the cgd platform module and is not intended to be used elsewhere
#endif /* P_INTERNAL_GUARD__ */

#include <core/int.h>
#include <core/pixel.h>
#include <core/shapes.h>
#include <stdbool.h>
#include <linux/fb.h>

struct window_fb {
    i32 fd;
    struct fb_fix_screeninfo fixed_info;
    struct fb_var_screeninfo var_info;

    void *mem;
    u64 mem_size;

    struct pixel_flat_data pixel_data;

    u32 xres, yres;
    u32 padding;

    rect_t win_area;

    bool closed;
};

i32 window_fb_open(struct window_fb *fb, const rect_t *area, const u32 flags);

void window_fb_close(struct window_fb *fb);

i32 window_fb_render_to_display(struct window_fb *fb);

void window_fb_bind_fb(struct window_fb *win, struct pixel_flat_data *fb);
void window_fb_unbind_fb(struct window_fb *win);

#endif /* P_WINDOW_FB_H_ */
