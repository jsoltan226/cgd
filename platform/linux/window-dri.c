#define _GNU_SOURCE
#define P_INTERNAL_GUARD__
#include "window-dri.h"
#undef P_INTERNAL_GUARD__
#include "../event.h"
#include "../window.h"
#include "../librtld.h"
#include <core/int.h>
#include <core/log.h>
#include <core/util.h>
#include <core/pixel.h>
#include <core/shapes.h>
#include <core/vector.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdatomic.h>
#include <poll.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <signal.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <drm.h>
#include <drm_mode.h>
#include <gbm.h>

#define MODULE_NAME "window-dri"

#define DRI_DEV_DIR "/dev/dri"
#define DRI_DEV_NAME_PREFIX "card"

struct file {
    i32 fd;
    char path[u_FILEPATH_MAX];
};

static VECTOR(struct file) open_available_devices(void);
static i32 dri_dev_scandir_filter(const struct dirent *d);

static i32 select_and_init_device(VECTOR(struct file) files,
    struct drm_device *dev, const struct libdrm_functions *drm);
static i32 select_connector_and_mode(i32 fd, u32 n_conns, u32 *conn_ids,
    drmModeConnectorPtr *o_conn, drmModeModeInfoPtr *o_mode,
    const struct libdrm_functions *drm);
static drmModeCrtcPtr find_crtc(i32 fd, drmModeResPtr res,
    drmModeConnectorPtr conn, const struct libdrm_functions *drm);

static void destroy_drm_device(struct drm_device *dev,
    const struct libdrm_functions *drm);

static i32 render_init_egl(struct egl_render_ctx *egl_rctx,
    struct gbm_device *gbm_dev, const struct drm_device *drm_dev,
    const struct libgbm_functions *gbm);
static void render_destroy_egl(struct egl_render_ctx *egl_rctx,
    const struct libgbm_functions *gbm);

static i32 render_init_software(struct software_render_ctx *sw_rctx,
    struct gbm_device *gbm_dev, const struct drm_device *drm_dev,
    const struct libdrm_functions *drm, const struct libgbm_functions *gbm);
static void render_destroy_software(struct software_render_ctx *sw_rctx,
    const struct libdrm_functions *drm, const struct libgbm_functions *gbm);

static void window_dri_render_software(struct window_dri *win,
    struct pixel_flat_data *fb);
static void window_dri_render_egl(struct window_dri *win);
static void perform_drm_page_flip(struct window_dri *win, u32 fb_handle);

#define OK 0
#define NOT_OK 1
static void finish_frame(struct window_dri_listener_thread *listener,
    bool status);

static void * window_dri_listener_fn(void *arg);
static void page_flip_handler(int fd, unsigned int frame,
    unsigned int tv_sec, unsigned int tv_usec, void *user_data);
static void window_dri_listener_signal_handler(i32 sig_num);

i32 window_dri_open(struct window_dri *win, const rect_t *area, const u32 flags)
{
    VECTOR(struct file) files = NULL;
    memset(win, 0, sizeof(struct window_dri));

    /* See if we can even use KMS at all (usually that isn't the case) */
    win->initialized_ = true;
    files = open_available_devices();
    if (files == NULL)
        goto_error("No available DRM devices were found.");

    /* Load libdrm */
    win->libdrm = p_librtld_load(LIBDRM_LIBNAME, libdrm_symnames);
    if (win->libdrm == NULL)
        goto_error("Unable to load libdrm");

#define X_(return_type, name, ...)                                  \
    win->drm.name = p_librtld_load_sym(win->libdrm, #name);         \
    if (win->drm.name == NULL)                                      \
        goto_error("Failed to load libdrm function \"%s\"", #name); \

    LIBDRM_FUNCTIONS_LIST
#undef X_
    win->drm.loaded_ = true;

    /* Check which device (if any) we should use */
    if (select_and_init_device(files, &win->dev, &win->drm))
        goto_error("No suitable DRM devices could be selected.");

    /* We don't need this anymore */
    vector_destroy(&files);

    /* Load and init libgbm */
    win->libgbm = p_librtld_load(LIBGBM_LIBNAME, libgbm_symnames);
    if (win->libgbm == NULL)
        goto_error("Unable to load libgbm");

#define X_(return_type, name, ...)                                  \
    win->gbm.name = p_librtld_load_sym(win->libgbm, #name);         \
    if (win->gbm.name == NULL)                                      \
        goto_error("Failed to load libgbm function \"%s\"", #name); \

    LIBGBM_FUNCTIONS_LIST
#undef X_
    win->gbm.loaded_ = true;

    win->gbm_dev = win->gbm.gbm_create_device(win->dev.fd);
    if (win->gbm_dev == NULL) {
        s_log_error("Failed to create GBM device.");
        return 1;
    }

    /* Init the window parameters */
    win->display_rect.x = win->display_rect.y = 0;
    win->display_rect.w = win->dev.width;
    win->display_rect.h = win->dev.height;

    memcpy(&win->win_rect, area, sizeof(rect_t));

    if (flags & P_WINDOW_POS_CENTERED_X)
        win->win_rect.x = (win->display_rect.w - win->win_rect.w) / 2;

    if (flags & P_WINDOW_POS_CENTERED_Y)
        win->win_rect.y = (win->display_rect.h - win->win_rect.h) / 2;

    /* By default, initialize software rendering (no acceleration) */
    win->acceleration = P_WINDOW_ACCELERATION_UNSET_;
    if (window_dri_set_acceleration(win, P_WINDOW_ACCELERATION_NONE))
        goto_error("Failed to init software rendering");

    /* Initialize the listener thread */
    win->listener.fd_p = &win->dev.fd;
    win->listener.acceleration_p = &win->acceleration;
    win->listener.drm = &win->drm;
    win->listener.gbm = &win->gbm;
    win->listener.render_ctx = &win->render;
    atomic_store(&win->listener.page_flip_pending, false);
    atomic_store(&win->listener.running, true);

    i32 ret = pthread_create(&win->listener.thread, NULL,
        window_dri_listener_fn, &win->listener);
    if (ret != 0) {
        atomic_store(&win->listener.running, false);
        goto_error("Failed to spawn the listener thread: %s.", strerror(ret));
    }

    return 0;

err:
    if (files != NULL) vector_destroy(&files);
    return 1;
}

void window_dri_close(struct window_dri *win)
{
    if (!win->initialized_) return;

    /* Take care of the listener thread */
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

    /* Clean up of acceleration-specific stuff */
    switch (win->acceleration) {
        case P_WINDOW_ACCELERATION_NONE:
            render_destroy_software(&win->render.sw, &win->drm, &win->gbm);
            break;
        case P_WINDOW_ACCELERATION_OPENGL:
            render_destroy_egl(&win->render.egl, &win->gbm);
            break;
        case P_WINDOW_ACCELERATION_VULKAN:
        default:
            break;
    }

    /* Close the device itself */
    if (win->dev.initialized_) {
        destroy_drm_device(&win->dev, &win->drm);
    }

    /* Finally, unload the libraries */
    if (win->libgbm != NULL)
        p_librtld_close(&win->libgbm);
    if (win->libdrm != NULL)
        p_librtld_close(&win->libdrm);

    memset(win, 0, sizeof(struct window_dri));
    win->dev.fd = -1;
}

void window_dri_render(struct window_dri *win, struct pixel_flat_data *fb)
{
    if (win->acceleration == P_WINDOW_ACCELERATION_OPENGL) {
        (void) fb;
        window_dri_render_egl(win);
    } else {
        window_dri_render_software(win, fb);
    }
}

i32 window_dri_set_acceleration(struct window_dri *win,
    enum p_window_acceleration val)
{
    u_check_params(win != NULL && val >= 0 && val < P_WINDOW_ACCELERATION_MAX_);

    if (win->acceleration == val) {
        s_log_warn("%s(): The desired acceleration mode is the same "
            "as the previous one; not doing anything.", __func__);
        return 0;
    }

    switch (win->acceleration) {
        case P_WINDOW_ACCELERATION_NONE:
            render_destroy_software(&win->render.sw, &win->drm, &win->gbm);
            break;
        case P_WINDOW_ACCELERATION_OPENGL:
            render_destroy_egl(&win->render.egl, &win->gbm);
            break;
        case P_WINDOW_ACCELERATION_UNSET_: /* previously unset, do nothing */
            break;
        case P_WINDOW_ACCELERATION_VULKAN:
        default:
            /* Shouldn't be possible */
            break;
    }

    win->acceleration = P_WINDOW_ACCELERATION_UNSET_;

    switch (val) {
    case P_WINDOW_ACCELERATION_NONE:
        if (render_init_software(&win->render.sw, win->gbm_dev, &win->dev,
            &win->drm, &win->gbm))
        {
            s_log_error("Failed to set the window up for software rendering.");
            return 1;
        }
        return 0;
    case P_WINDOW_ACCELERATION_OPENGL:
        if (render_init_egl(&win->render.egl,
            win->gbm_dev, &win->dev, &win->gbm))
        {
            s_log_error("Failed to set the window up for EGL rendering.");
            return 1;
        }
        return 0;
    case P_WINDOW_ACCELERATION_VULKAN:
        s_log_error("Vulkan acceleration not implemented yet.");
        return 1;
        break;
    default: /* Technically not possible */
        s_log_error("Unsupported acceleration mode: %u", val);
        return 1;
    }

    win->acceleration = val;
}

void window_dri_set_egl_buffers_swapped(struct window_dri *win)
{
    u_check_params(win != NULL);
    s_assert(win->acceleration == P_WINDOW_ACCELERATION_OPENGL,
        "OpenGL acceleration must be enabled to swap EGL buffers");
    atomic_store(&win->render.egl.buffers_swapped, true);
}

static VECTOR(struct file) open_available_devices(void)
{
    struct dirent **namelist = NULL;
    i32 n_dirents = scandir(DRI_DEV_DIR, &namelist,
        &dri_dev_scandir_filter, &alphasort);
    if (n_dirents == -1) {
        s_log_error("Failed to scan directory \"%s\": %s",
            DRI_DEV_DIR, strerror(errno));
        return NULL;
    } else if (n_dirents == 0) {
        u_nfree(&namelist);
        return NULL;
    }

    VECTOR(struct file) files = vector_new(struct file);

    for (u32 i = 0; i < n_dirents; i++) {
        struct file f = { .fd = -1, .path = { 0 } };

        strncpy(f.path, DRI_DEV_DIR "/", u_FILEPATH_MAX - 1);
        strncat(f.path, namelist[i]->d_name,
            u_FILEPATH_MAX - strlen(f.path) - 1);
        f.path[u_FILEPATH_MAX - 1] = '\0'; /* just in case */

        f.fd = open(f.path, O_RDWR);
        if (f.fd == -1) {
            u_nfree(&namelist[i]);
            continue;
        }

        struct stat s;
        if (fstat(f.fd, &s)) {
            s_log_error("Can't stat \"%s\" (even after opening): %s",
                f.path, strerror(errno));
            close(f.fd);
            u_nfree(&namelist[i]);
            continue;
        }

        if (!S_ISCHR(s.st_mode)) {
            /* Not a character device */
            close(f.fd);
            u_nfree(&namelist[i]);
            continue;
        }

        vector_push_back(files, f);
        u_nfree(&namelist[i]);
    }
    u_nfree(&namelist);

    if (vector_size(files) == 0) {
        vector_destroy(&files);
        return NULL;
    }

    return files;
}

static i32 dri_dev_scandir_filter(const struct dirent *d)
{
    return !strncmp(d->d_name, DRI_DEV_NAME_PREFIX,
        u_strlen(DRI_DEV_NAME_PREFIX));
}

static i32 select_and_init_device(VECTOR(struct file) files,
    struct drm_device *dev, const struct libdrm_functions *drm)
{
    u32 top_score = 0;
    struct drm_device curr_best_dev = { 0 }, tmp_dev = { 0 };
    memset(dev, 0, sizeof(struct drm_device));

    for (u32 i = 0; i < vector_size(files); i++) {
        memset(&tmp_dev, 0, sizeof(struct drm_device));
        tmp_dev.initialized_ = true;
        tmp_dev.fd = files[i].fd;
        strncpy(tmp_dev.path, files[i].path, u_FILEPATH_MAX);

        /* Get the resources */
        drmModeResPtr res = drm->drmModeGetResources(tmp_dev.fd);
        tmp_dev.res = res;
        if (res == NULL) {
            destroy_drm_device(&tmp_dev, drm);
            continue;
        }

        /* Select the connector */
        if (select_connector_and_mode(tmp_dev.fd,
            res->count_connectors, res->connectors,
            &tmp_dev.conn, &tmp_dev.mode, drm))
        {
            destroy_drm_device(&tmp_dev, drm);
            continue;
        }

        /* Determine whether we should even proceed
         * (the current option must be the best one) */
        tmp_dev.width = tmp_dev.mode->hdisplay;
        tmp_dev.height = tmp_dev.mode->vdisplay;
        tmp_dev.refresh_rate = tmp_dev.mode->vrefresh;

        u32 score = tmp_dev.width * tmp_dev.height * tmp_dev.refresh_rate;
        if (score <= top_score) {
            destroy_drm_device(&tmp_dev, drm);
            continue;
        }

        /* Find a suitable CRTC */
        tmp_dev.crtc = find_crtc(tmp_dev.fd, res, tmp_dev.conn, drm);
        if (tmp_dev.crtc == NULL) {
            destroy_drm_device(&tmp_dev, drm);
            continue;
        }

        /* (Try to) become the DRM master */
        if (drm->drmSetMaster(tmp_dev.fd)) {
            s_log_warn("Failed to become the DRM master: %s. "
                "Make sure that you have the neccessary permissions "
                "and that no other process already is the DRM master "
                "(i.e. you aren't running under an active X11 or Wayland "
                "session)",
                strerror(errno));
            destroy_drm_device(&tmp_dev, drm);
            continue;
        }

        memcpy(&curr_best_dev, &tmp_dev, sizeof(struct drm_device));
    }

    if (curr_best_dev.initialized_) {
        memcpy(dev, &curr_best_dev, sizeof(struct drm_device));
        s_log_debug("Selected device \"%s\" with mode %ux%u@%uHz",
            dev->path, dev->width, dev->height, dev->refresh_rate);
        return 0;
    } else {
        return 1;
    }
}

static i32 select_connector_and_mode(i32 fd, u32 n_conns, u32 *conn_ids,
    drmModeConnectorPtr *o_conn, drmModeModeInfoPtr *o_mode,
    const struct libdrm_functions *drm)
{
    drmModeConnectorPtr ret_conn = NULL;
    drmModeModeInfoPtr ret_mode = NULL;
    u32 top_score = 0;

    u32 i = 0;
    for (; i < n_conns; i++) {
        drmModeConnectorPtr conn = drm->drmModeGetConnector(fd, conn_ids[i]);

        if (conn == NULL) {
            continue;
        } else if (conn->connection != DRM_MODE_CONNECTED ||
            conn->count_modes < 1)
        {
            drm->drmModeFreeConnector(conn);
            continue;
        }

        /* The first mode is usually the "preffered" one */
        drmModeModeInfoPtr best_mode_ptr = &conn->modes[0];
        ret_mode = best_mode_ptr;
        u32 top_mode_score = 0;
        for (u32 i = 0; i < conn->count_modes; i++) {
            /* Score = Resolution * Refresh Rate */
            const u32 curr_score = conn->modes[i].vrefresh *
                conn->modes[i].hdisplay * conn->modes[i].vdisplay;

            if (curr_score > top_mode_score) {
                best_mode_ptr = &conn->modes[i];
                top_mode_score = curr_score;
            }
        }

        if (top_mode_score > top_score) {
            top_score = top_mode_score;
            if (ret_conn != NULL)
                drm->drmModeFreeConnector(ret_conn);
            ret_conn = conn;
            ret_mode = best_mode_ptr;
        } else {
            drm->drmModeFreeConnector(conn);
        }
    }

    if (ret_conn != NULL) {
        *o_conn = ret_conn;
        *o_mode = ret_mode;
        return 0;
    } else {
        *o_conn = NULL;
        *o_mode = NULL;
        return 1;
    }
}

static drmModeCrtcPtr find_crtc(i32 fd, drmModeResPtr res,
    drmModeConnectorPtr conn, const struct libdrm_functions *drm)
{
    drmModeEncoderPtr enc = NULL;
    drmModeCrtcPtr ret = NULL;

    /* First, see if the connector's default encoder+CRTC are OK */
    if (conn->encoder_id) {
        enc = drm->drmModeGetEncoder(fd, conn->encoder_id);
        if (enc != NULL && enc->crtc_id) {
            ret = drm->drmModeGetCrtc(fd, enc->crtc_id);
            if (ret != NULL) {
                drm->drmModeFreeEncoder(enc);
                return ret;
            }
        }
    }
    if (enc != NULL) {
        drm->drmModeFreeEncoder(enc);
        enc = NULL;
    }

    /* If no Encoder/CRTC was attached to the connector,
     * find a suitable one in it's encoder list */
    for (u32 i = 0; i < conn->count_encoders; i++) {
        enc = drm->drmModeGetEncoder(fd, conn->encoders[i]);
        if (enc == NULL)
            continue;

        for (u32 j = 0; j < res->count_crtcs; j++) {
            if (enc->possible_crtcs & (1 << j)) {
                /* We found a compatible CRTC! Yay! */
                u32 crtc_id = res->crtcs[j];
                ret = drm->drmModeGetCrtc(fd, crtc_id);
                if (ret != NULL) {
                    drm->drmModeFreeEncoder(enc);
                    return ret;
                }
            }
        }

        drm->drmModeFreeEncoder(enc);
    }

    return NULL;
}

static void destroy_drm_device(struct drm_device *dev,
    const struct libdrm_functions *drm)
{
    if (dev == NULL) return;

    if (dev->crtc != NULL) {
        /* Restore the previous CRTC configuration */
        (void) drm->drmModeSetCrtc(dev->fd, dev->crtc->crtc_id,
            dev->crtc->buffer_id, dev->crtc->x, dev->crtc->y,
            &dev->conn->connector_id, 1, &dev->crtc->mode);
        drm->drmModeFreeCrtc(dev->crtc);
        dev->crtc = NULL;
    }
    (void) drm->drmDropMaster(dev->fd);
    if (dev->conn != NULL) {
        drm->drmModeFreeConnector(dev->conn);
        dev->conn = NULL;
    }
    dev->mode = NULL;
    if (dev->res != NULL) {
        drm->drmModeFreeResources(dev->res);
        dev->res = NULL;
    }
    if (dev->fd != -1) {
        close(dev->fd);
        dev->fd = -1;
    }

    dev->width = dev->height = dev->refresh_rate = 0;
}

static i32 render_init_egl(struct egl_render_ctx *egl_rctx,
    struct gbm_device *gbm_dev, const struct drm_device *drm_dev,
    const struct libgbm_functions *gbm)
{
    memset(egl_rctx, 0, sizeof(struct egl_render_ctx));
    egl_rctx->initialized_ = true;

    atomic_store(&egl_rctx->buffers_swapped, false);

    egl_rctx->surface = gbm->gbm_surface_create(gbm_dev,
        drm_dev->width, drm_dev->height, GBM_BO_FORMAT_XRGB8888,
        GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);
    if (egl_rctx->surface == NULL) {
        s_log_error("Failed to create GBM surface.");
        return 1;
    }

    egl_rctx->curr_bo = egl_rctx->next_bo = NULL;
    egl_rctx->curr_fb_id = egl_rctx->next_fb_id = 0;
    atomic_store(&egl_rctx->front_buffer_in_use, false);

    return 0;
}

static void render_destroy_egl(struct egl_render_ctx *egl_rctx,
    const struct libgbm_functions *gbm)
{
    if (!egl_rctx->initialized_) return;

    if (egl_rctx->surface != NULL) {
        gbm->gbm_surface_destroy(egl_rctx->surface);
    }

    memset(egl_rctx, 0, sizeof(struct egl_render_ctx));
}

static i32 render_init_software(struct software_render_ctx *sw_rctx,
    struct gbm_device *gbm_dev, const struct drm_device *drm_dev,
    const struct libdrm_functions *drm, const struct libgbm_functions *gbm)
{
    memset(sw_rctx, 0, sizeof(struct software_render_ctx));
    sw_rctx->initialized_ = true;
    sw_rctx->fb_id = -1;

    sw_rctx->bo = gbm->gbm_bo_create(gbm_dev, drm_dev->width, drm_dev->height,
        GBM_BO_FORMAT_XRGB8888, GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);
    if (sw_rctx->bo == NULL) {
        s_log_error("Failed to create GBM buffer object");
        return 1;
    }

    const u32 stride = gbm->gbm_bo_get_stride(sw_rctx->bo);
    const u32 handle = gbm->gbm_bo_get_handle(sw_rctx->bo).u32;
    i32 ret = drm->drmModeAddFB(drm_dev->fd, drm_dev->width, drm_dev->height,
        24, 32, stride, handle, &sw_rctx->fb_id);
    if (ret != 0) {
        s_log_error("Failed to add framebuffer: %s", strerror(errno));
        gbm->gbm_bo_destroy(sw_rctx->bo);
        sw_rctx->bo = NULL;
        sw_rctx->fb_id = -1;
        return 1;
    }

    ret = drm->drmModeSetCrtc(drm_dev->fd, drm_dev->crtc->crtc_id,
        sw_rctx->fb_id, 0, 0, &drm_dev->conn->connector_id, 1,
        drm_dev->mode);
    if (ret != 0) {
        s_log_error("Failed to set CRTC: %s", strerror(errno));
        (void) drm->drmModeRmFB(drm_dev->fd, sw_rctx->fb_id);
        gbm->gbm_bo_destroy(sw_rctx->bo);
        sw_rctx->bo = NULL;
        sw_rctx->fb_id = -1;
        return 1;
    }

    return 0;
}

static void render_destroy_software(struct software_render_ctx *sw_rctx,
    const struct libdrm_functions *drm, const struct libgbm_functions *gbm)
{
    if (!sw_rctx->initialized_) return;

    if (sw_rctx->bo != NULL) {
        if (sw_rctx->fb_id != -1) {
            i32 fd = gbm->gbm_bo_get_fd(sw_rctx->bo);
            (void) drm->drmModeRmFB(fd, sw_rctx->fb_id);
        }
        gbm->gbm_bo_destroy(sw_rctx->bo);
    }

    memset(sw_rctx, 0, sizeof(struct software_render_ctx));
    sw_rctx->fb_id = -1;
}

static void window_dri_render_software(struct window_dri *win,
    struct pixel_flat_data *fb)
{
    s_assert(fb != NULL && fb->buf != NULL,
        "No buffer provided in software-renderered window");

    u32 *map = NULL;
    void *map_data = NULL;
    u32 stride = 0;
    map = win->gbm.gbm_bo_map(win->render.sw.bo,
        0, 0, win->dev.width, win->dev.height,
        GBM_BO_TRANSFER_WRITE, &stride, &map_data
    );
    s_assert(map != NULL, "Failed to map GBM buffer: %s", strerror(errno));

    /* Clip the window rect to be within the screen */
    rect_t dst = win->win_rect;
    rect_clip(&dst, &win->display_rect);

    /* "Project" the changes made to dst onto src */
    const rect_t src = {
        .x = -(win->win_rect.x - dst.x),
        .y = -(win->win_rect.y - dst.y),
        .w = fb->w - (win->win_rect.w - dst.w),
        .h = fb->h - (win->win_rect.h - dst.h),
    };
    for (u32 y = 0; y < src.h; y++) {
        const u32 dst_offset = (
            (dst.y + y) * (stride / 4)
            + dst.x
        );
        const u32 src_offset = (
            ((src.y + y) * src.w)
            + src.x
        );

        memcpy(
            map + dst_offset,
            fb->buf + src_offset,
            src.w * sizeof(pixel_t)
        );
    }

    win->gbm.gbm_bo_unmap(win->render.sw.bo, map_data);

    i32 ret = win->drm.drmModeSetCrtc(win->dev.fd, win->dev.crtc->crtc_id,
        win->render.sw.fb_id, 0, 0, &win->dev.conn->connector_id, 1,
        win->dev.mode);
    if (ret != 0) {
        s_log_error("Failed to set CRTC: %s", strerror(errno));
        return;
    }

    perform_drm_page_flip(win, win->render.sw.fb_id);
}

static void window_dri_render_egl(struct window_dri *win)
{
    /* Sanity checks */
    if (atomic_load(&win->render.egl.front_buffer_in_use)) {
        s_log_error("Another frame is being displayed right now! "
            "Dropping this one!");
        return;
    }
    atomic_store(&win->render.egl.front_buffer_in_use, true);

    if (!atomic_load(&win->render.egl.buffers_swapped)) {
        s_log_error("Attempt to present framebuffer without first swapping it; "
            "dropping this frame.");
        return;
    }
    win->render.egl.buffers_swapped = false;

    if (!win->gbm.gbm_surface_has_free_buffers(win->render.egl.surface)) {
        s_log_error("GBM surface has no free buffers left; dropping frame.");
        return;
    }

    /* Get the new front buffer... */
    struct gbm_bo *bo =
        win->gbm.gbm_surface_lock_front_buffer(win->render.egl.surface);
    s_assert(bo != NULL, "Failed to lock GBM front buffer");
    win->render.egl.next_bo = bo;

    /* ...And add it to our CRTC */
    const u32 stride = win->gbm.gbm_bo_get_stride(bo);
    const u32 handle = win->gbm.gbm_bo_get_handle(bo).u32;
    i32 ret = win->drm.drmModeAddFB(win->dev.fd,
        win->dev.width, win->dev.height, 24, 32, stride, handle,
        &win->render.egl.next_fb_id);
    if (ret != 0) {
        s_log_error("Failed to add framebuffer: %s. Dropping this frame.",
            strerror(errno));
        win->gbm.gbm_surface_release_buffer(win->render.egl.surface, bo);
        win->render.egl.curr_bo = NULL;
        atomic_store(&win->render.egl.front_buffer_in_use, false);
        return;
    }

    /* And finally, schedule the pageflip for the next VBlank */
    perform_drm_page_flip(win, win->render.egl.next_fb_id);
}

static void perform_drm_page_flip(struct window_dri *win, u32 fb_handle)
{
    if (atomic_load(&win->listener.page_flip_pending)) {
        s_log_warn("Another page flip already scheduled! Dropping this frame.");
        return;
    }
    atomic_store(&win->listener.page_flip_pending, true);

    i32 ret = win->drm.drmModePageFlip(win->dev.fd, win->dev.crtc->crtc_id,
        fb_handle, DRM_MODE_PAGE_FLIP_EVENT, &win->listener);
    if (ret != 0) {
        s_log_error("Failed to schedule a page flip: %s", strerror(errno));

        /* Interrupt the listener thread */
        (void) pthread_kill(win->listener.thread, SIGUSR1);
        return;
    }
    s_log_debug("Scheduled page flip for next VBlank");
}

static void finish_frame(struct window_dri_listener_thread *listener,
    bool status)
{
    s_assert(listener->fd_p != NULL && *listener->fd_p != -1,
        "The DRM device file descriptor is not initialized!");
    s_assert(listener->acceleration_p != NULL,
        "The acceleration mode pointer is not initialized!");
    s_assert(listener->drm != NULL && listener->drm->loaded_,
        "The libdrm functions are not initialized");;
    s_assert(listener->gbm != NULL && listener->gbm->loaded_,
        "The libgbm functions are not initialized");;

    /* Clean everything up */
    if (*listener->acceleration_p == P_WINDOW_ACCELERATION_OPENGL) {

        /* Clean up the previous frame */
        struct egl_render_ctx *egl_rctx = &listener->render_ctx->egl;

        if (egl_rctx->curr_fb_id != 0) {
            (void) listener->drm->drmModeRmFB(
                *listener->fd_p, egl_rctx->curr_fb_id
            );
            egl_rctx->curr_fb_id = 0;
        }
        if (egl_rctx->curr_bo != NULL) {
            (void) listener->gbm->gbm_surface_release_buffer(egl_rctx->surface,
                egl_rctx->curr_bo);
            egl_rctx->curr_bo = NULL;
        }

        egl_rctx->curr_bo = egl_rctx->next_bo;
        egl_rctx->curr_fb_id = egl_rctx->next_fb_id;

        atomic_store(&listener->render_ctx->egl.front_buffer_in_use, false);
    }

    /* Inform everyone about the page flip */
    atomic_store(&listener->page_flip_pending, false);

    const struct p_event ev = {
        .type = P_EVENT_PAGE_FLIP,
        .info.page_flip_status = status,
    };
    p_event_send(&ev);
}

static void * window_dri_listener_fn(void *arg)
{
    struct window_dri_listener_thread *listener = arg;

    /* Only allow SIGUSR1 */
    sigset_t sig_mask;
    sigfillset(&sig_mask);
    sigdelset(&sig_mask, SIGUSR1);
    i32 ret = pthread_sigmask(SIG_SETMASK, &sig_mask, NULL);
    s_assert(ret == 0, "Failed to set the listener thread's signal mask: %s",
        strerror(ret));

    /* Set the signal handler for SIGUSR1 */
    struct sigaction sa = { 0 };
    sa.sa_handler = window_dri_listener_signal_handler;
    sa.sa_flags = 0;
    /* Block all other signals while the handler is running */
    sigfillset(&sa.sa_mask);

    /* Should only fail with EINVAL (invalid argument) */
    s_assert(sigaction(SIGUSR1, &sa, NULL) == 0,
        "Failed to set the SIGUSR1 handler: %s", strerror(errno));

    s_assert(listener->fd_p != NULL && *listener->fd_p != -1,
        "The device file descriptor is invalid");

    s_assert(listener->drm != NULL && listener->drm->loaded_,
        "The libdrm functions are not initialized");;
    s_assert(listener->gbm != NULL && listener->gbm->loaded_,
        "The libgbm functions are not initialized");;

    struct pollfd poll_fd = {
        .fd = *listener->fd_p,
        .events = POLLIN,
    };

    drmEventContext ev_ctx = {
        .version = DRM_EVENT_CONTEXT_VERSION,
        .page_flip_handler = page_flip_handler,
    };

    while (atomic_load(&listener->running)) {
        s_assert(listener->fd_p != NULL && *listener->fd_p != -1,
            "The device file descriptor is invalid");

        ret = poll(&poll_fd, 1, -1);
        if (ret > 0 && poll_fd.revents & POLLIN) {

            /* If everything is OK, `finish_frame` will be called
             * in the page flip handler */
            if (listener->drm->drmHandleEvent(*listener->fd_p, &ev_ctx) != 0) {
                s_log_error("drmHandleEvent failed: I/O error");
                finish_frame(listener, NOT_OK);
            }
        } else if (ret == 0 && atomic_load(&listener->page_flip_pending)) {
            /* The fd isn't ready for I/O,
             * which means that the timeout expired */
            s_log_warn("Timeout for vblank expired; dropping frame");
            finish_frame(listener, NOT_OK);
        } else if (ret == -1) {
            if (errno == EINTR) { /* Interrupted by signal */
                s_log_debug("%s: poll() interrupted by signal", __func__);
                if (atomic_load(&listener->page_flip_pending)) {
                    s_log_debug("Dropping current frame due to interruption.");
                    finish_frame(listener, NOT_OK);
                }
                continue;
            } else {
                s_log_error("Failed to poll() on the DRM device: %s",
                    strerror(errno));
            }
        }
    }

    pthread_exit(NULL);
}

static void page_flip_handler(int fd, unsigned int frame,
    unsigned int tv_sec, unsigned int tv_usec, void *user_data)
{
    struct window_dri_listener_thread *listener = user_data;

    if (atomic_load(&listener->page_flip_pending) != true) {
        s_log_error("%s: Nothing is waiting for the vblank event "
            "(or another frame is being displayed)!", __func__);
        finish_frame(listener, NOT_OK);
        return;
    }

    finish_frame(listener, OK);
}

static void window_dri_listener_signal_handler(i32 sig_num)
{
    (void) sig_num;
    /* Do nothing lol */
}
