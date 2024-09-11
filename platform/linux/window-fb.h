#ifndef P_WINDOW_FB_H_
#define P_WINDOW_FB_H_

#ifndef P_INTERNAL_GUARD__
#error This header file is internal to the cgd platform module and is not intended to be used elsewhere
#endif /* P_INTERNAL_GUARD__ */

#include "core/pixel.h"
#include "core/shapes.h"
#include "core/int.h"
#include <linux/fb.h>

struct window_fb {
    i32 fd;
    struct fb_fix_screeninfo fixed_info;
    struct fb_var_screeninfo var_info;

    void *mem;
    u64 mem_size;

    u32 xres, yres;
    u32 padding;
};

i32 window_fb_open(struct window_fb *fb, const rect_t *area);

void window_fb_close(struct window_fb *fb);

i32 window_fb_render_to_display(struct window_fb *fb,
    const pixel_t *frame, const rect_t *area);

#endif /* P_WINDOW_FB_H_ */
