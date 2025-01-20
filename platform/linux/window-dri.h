#ifndef P_WINDOW_DRI_H_
#define P_WINDOW_DRI_H_

#ifndef P_INTERNAL_GUARD__
#error This header file is internal to the cgd platform module and is not intended to be used elsewhere
#endif /* P_INTERNAL_GUARD__ */

#define P_INTERNAL_GUARD__
#include "window-acceleration.h"
#undef P_INTERNAL_GUARD__
#include <core/int.h>
#include <core/util.h>
#include <core/pixel.h>
#include <core/shapes.h>
#include <stdbool.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <pthread.h> /* We need the signal functionality */

#define LIBDRM_LIBNAME "libdrm"
#define LIBDRM_FUNCTIONS_LIST                                               \
    X_(drmModeResPtr, drmModeGetResources, int fd)                          \
    X_(int, drmSetMaster, int fd)                                           \
    X_(drmModeConnectorPtr, drmModeGetConnector, int fd, uint32_t conn_id)  \
    X_(void, drmModeFreeConnector, drmModeConnectorPtr ptr)                 \
    X_(drmModeEncoderPtr, drmModeGetEncoder, int fd, uint32_t encoder_id)   \
    X_(drmModeCrtcPtr, drmModeGetCrtc, int fd, uint32_t crtcId)             \
    X_(void, drmModeFreeEncoder, drmModeEncoderPtr ptr)                     \
    X_(int, drmModeSetCrtc,                                                 \
        int fd, uint32_t crtcId, uint32_t bufferId, uint32_t x, uint32_t y, \
        uint32_t *connectors, int count, drmModeModeInfoPtr mode            \
    )                                                                       \
    X_(int, drmDropMaster, int fd)                                          \
    X_(void, drmModeFreeCrtc, drmModeCrtcPtr ptr)                           \
    X_(void, drmModeFreeResources, drmModeResPtr ptr)                       \
    X_(int, drmModeAddFB,                                                   \
        int fd, uint32_t width, uint32_t height, uint8_t depth,             \
        uint8_t bpp, uint32_t pitch, uint32_t bo_handle, uint32_t *buf_id   \
    )                                                                       \
    X_(int, drmModeRmFB, int fd, uint32_t bufferId)                         \
    X_(int, drmModePageFlip,                                                \
        int fd, uint32_t crtc_id, uint32_t fb_id,                           \
        uint32_t flags, void *user_data                                     \
    )                                                                       \
    X_(int, drmHandleEvent, int fd, drmEventContextPtr evctx)               \


#define LIBGBM_LIBNAME "libgbm"
#define LIBGBM_FUNCTIONS_LIST                                               \
    X_(struct gbm_device *, gbm_create_device, int fd)                      \
    X_(struct gbm_surface *, gbm_surface_create,                            \
        struct gbm_device *gbm, uint32_t width, uint32_t height,            \
        uint32_t format, uint32_t flags                                     \
    )                                                                       \
    X_(void, gbm_surface_destroy, struct gbm_surface *surface)              \
    X_(struct gbm_bo *, gbm_bo_create,                                      \
        struct gbm_device *gbm, uint32_t width, uint32_t height,            \
        uint32_t format, uint32_t flags                                     \
    )                                                                       \
    X_(uint32_t, gbm_bo_get_stride, struct gbm_bo *bo)                      \
    X_(union gbm_bo_handle, gbm_bo_get_handle, struct gbm_bo *bo)           \
    X_(void, gbm_bo_destroy, struct gbm_bo *bo)                             \
    X_(int, gbm_bo_get_fd, struct gbm_bo *bo)                               \
    X_(void *, gbm_bo_map,                                                  \
        struct gbm_bo *bo, uint32_t x, uint32_t y,                          \
        uint32_t width, uint32_t height, uint32_t flags,                    \
        uint32_t *stride, void **map_data                                   \
    )                                                                       \
    X_(void, gbm_bo_unmap, struct gbm_bo *bo, void *map_data)               \
    X_(int, gbm_surface_has_free_buffers, struct gbm_surface *surface)      \
    X_(struct gbm_bo *, gbm_surface_lock_front_buffer,                      \
        struct gbm_surface *surface                                         \
    )                                                                       \
    X_(void, gbm_surface_release_buffer,                                    \
        struct gbm_surface *surface, struct gbm_bo *bo                      \
    )                                                                       \

#define X_(return_type, name, ...) return_type(*name)(__VA_ARGS__);
struct libdrm_functions {
    LIBDRM_FUNCTIONS_LIST
    bool loaded_;
};

struct libgbm_functions {
    LIBGBM_FUNCTIONS_LIST
    bool loaded_;
};
#undef X_

#define X_(return_type, name, ...) #name,
static const char *const libdrm_symnames[] = {
    LIBDRM_FUNCTIONS_LIST
    NULL
};

static const char *const libgbm_symnames[] = {
    LIBGBM_FUNCTIONS_LIST
    NULL
};
#undef X_

struct drm_device {
    i32 fd;
    char path[u_FILEPATH_MAX];

    drmModeResPtr res;
    drmModeConnectorPtr conn;
    drmModeModeInfoPtr mode;
    drmModeCrtcPtr crtc;

    u32 width, height;
    u32 refresh_rate;

    bool initialized_;
};

struct software_render_ctx {
    struct gbm_bo *bo;
    u32 fb_id;
    bool initialized_;
};
struct egl_render_ctx {
    struct gbm_surface *surface;

    _Atomic bool front_buffer_in_use;
    u32 curr_fb_id, next_fb_id;
    struct gbm_bo *curr_bo, *next_bo;

    _Atomic bool buffers_swapped;
    bool initialized_;
};

struct window_dri_listener_thread {
    pthread_t thread;
    _Atomic bool running;

    /* The values of these should be read-only for the thread */
    const i32 *fd_p;
    const enum window_acceleration *accelerated_p;
    const struct libdrm_functions *drm;
    const struct libgbm_functions *gbm;

    _Atomic bool page_flip_pending;

    union window_dri_render_ctx *render_ctx;
};

struct window_dri {
    struct p_lib *libdrm, *libgbm;
    struct libdrm_functions drm;
    struct libgbm_functions gbm;

    struct drm_device dev;

    enum window_acceleration accelerated;
    struct gbm_device *gbm_dev;
    union window_dri_render_ctx {
        struct software_render_ctx sw;
        struct egl_render_ctx egl;
    } render;

    rect_t win_rect;
    rect_t display_rect;

    struct window_dri_listener_thread listener;

    bool initialized_;
};

i32 window_dri_open(struct window_dri *win,
    const rect_t *area, const u32 flags);

void window_dri_close(struct window_dri *win);

void window_dri_render(struct window_dri *win, struct pixel_flat_data *fb);

void window_dri_set_acceleration(struct window_dri *win,
    enum window_acceleration val);
void window_dri_set_egl_buffers_swapped(struct window_dri *win);

#endif /* P_WINDOW_DRI_H_ */
