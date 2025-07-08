#ifndef WINDOW_PRESENT_SOFTWARE_H_
#define WINDOW_PRESENT_SOFTWARE_H_

#ifndef P_INTERNAL_GUARD__
#error This header file is internal to the cgd platform module and is not intended to be used elsewhere
#endif /* P_INTERNAL_GUARD__ */

#include "../window.h"
#include <core/int.h>
#include <core/pixel.h>
#include <stdbool.h>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif /* WIN32_LEAN_AND_MEAN */
#include <windows.h>
#include <wingdi.h>

struct window_render_software_ctx {
    bool initialized_;

    HDC dc; /* A handle to the device context of the window */
    HWND win_handle; /* A handle to the window (used for handling the DC) */

    /* The pixel buffers to which everything is rendered */
    struct pixel_flat_data buffers_[2];
    struct pixel_flat_data *back_fb, *front_fb;
};

struct render_init_software_req {
    struct window_render_software_ctx *ctx;
    const struct p_window_info *win_info;
    HWND win_handle;
};
i32 render_init_software(struct window_render_software_ctx *ctx,
    const struct p_window_info *win_info, HWND win_handle);

struct render_present_software_req {
    struct window_render_software_ctx *ctx;

    struct pixel_flat_data *o_new_back_buf;
};
struct pixel_flat_data *
render_present_software(struct window_render_software_ctx *ctx);

struct render_destroy_software_req {
    struct window_render_software_ctx *ctx;
};
void render_destroy_software(struct window_render_software_ctx *ctx);

#endif /* WINDOW_PRESENT_SOFTWARE_H_ */
