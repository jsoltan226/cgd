#ifndef P_WINDOW_DRI_H_
#define P_WINDOW_DRI_H_

#ifndef P_INTERNAL_GUARD__
#error This header file is internal to the cgd platform module and is not intended to be used elsewhere
#endif /* P_INTERNAL_GUARD__ */

#include <core/int.h>
#include <core/pixel.h>
#include <core/shapes.h>
#include <stdbool.h>
#include <xf86drmMode.h>

struct drm_device {
    i32 fd;
    drmModeResPtr res;
    drmModeConnectorPtr conn;
    drmModeCrtcPtr crtc;

    u32 width, height;
    u32 refresh_rate;

    bool initialized_;
};

struct window_dri {
    struct drm_device dev;

    struct gbm_device *gbm_dev;
};

i32 window_dri_open(struct window_dri *win,
    const rect_t *area, const u32 flags);

void window_dri_close(struct window_dri *win);

void window_dri_render(struct window_dri *win, struct pixel_flat_data *fb);

#endif /* P_WINDOW_DRI_H_ */
