#define _GNU_SOURCE
#define P_INTERNAL_GUARD__
#include "window-dri.h"
#undef P_INTERNAL_GUARD__
#include <core/int.h>
#include <core/log.h>
#include <core/util.h>
#include <core/pixel.h>
#include <core/shapes.h>
#include <core/vector.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <gbm.h>

#define MODULE_NAME "window-dri"

#define DRI_DEV_DIR "/dev/dri"
#define DRI_DEV_NAME_PREFIX "card"

static VECTOR(i32) open_available_devices(void);
static i32 select_and_init_device(VECTOR(i32) fds, struct drm_device *dev);
static drmModeConnectorPtr select_connector(i32 fd, u32 n_conns, u32 *conn_ids);
static drmModeCrtcPtr find_crtc(i32 fd, drmModeResPtr res,
    drmModeConnectorPtr conn);
static i32 dri_dev_scandir_filter(const struct dirent *d);
static void destroy_drm_device(struct drm_device *dev);

i32 window_dri_open(struct window_dri *win, const rect_t *area, const u32 flags)
{
    VECTOR(i32) fds = NULL;
    memset(win, 0, sizeof(struct window_dri));

    fds = open_available_devices();
    if (fds == NULL)
        goto_error("No available DRM devices were found.");

    if (select_and_init_device(fds, &win->dev))
        goto_error("No suitable DRM devices could be selected.");


    /* If we can't have graphics acceleration,
     * we might as well fall back to window-fbdev lol */
    win->gbm_dev = gbm_create_device(win->dev.fd);
    if (win->gbm_dev == NULL)
        goto_error("Failed to create gbm device.");

    vector_destroy(&fds);

    return 0;

err:
    if (fds != NULL) vector_destroy(&fds);
    return 1;
}

void window_dri_close(struct window_dri *win)
{
    if (win->gbm_dev != NULL) {
        gbm_device_destroy(win->gbm_dev);
    }
    if (win->dev.initialized_) {
        destroy_drm_device(&win->dev);
    }
    memset(win, 0, sizeof(struct window_dri));
}

void window_dri_render(struct window_dri *win, struct pixel_flat_data *fb)
{
    // TODO
}

static VECTOR(i32) open_available_devices(void)
{
    struct dirent **namelist = NULL;
    i32 n_dirents = scandir(DRI_DEV_DIR, &namelist,
        &dri_dev_scandir_filter, &alphasort);
    if (n_dirents == -1) {
        s_log_error("Failed to scan directory \"%s\": %s",
            DRI_DEV_DIR, strerror(errno));
        return NULL;
    } else if (n_dirents == 0) {
        return NULL;
    }

    VECTOR(i32) devices = vector_new(i32);

    for (u32 i = 0; i < n_dirents; i++) {
        i32 fd = -1;

        const u32 pathbuf_size = u_strlen(DRI_DEV_DIR "/") +
            strlen(namelist[i]->d_name) + 1;
        char pathbuf[pathbuf_size];
        (void) snprintf(pathbuf, pathbuf_size,
            "%s/%s", DRI_DEV_DIR, namelist[i]->d_name);
        pathbuf[pathbuf_size - 1] = '\0';

        fd = open(pathbuf, O_RDWR);
        if (fd == -1)
            continue;

        struct stat s;
        if (fstat(fd, &s)) {
            s_log_error("Can't stat \"%s\" (even after opening): %s",
                pathbuf, strerror(errno));
            close(fd);
            continue;
        }

        if (!S_ISCHR(s.st_mode)) {
            /* Not a character device */
            close(fd);
            continue;
        }

        vector_push_back(devices, fd);
    }

    if (vector_size(devices) == 0) {
        vector_destroy(&devices);
        return NULL;
    }

    return devices;
}

static i32 select_and_init_device(VECTOR(i32) fds, struct drm_device *dev)
{
    u32 top_score = 0;
    struct drm_device curr_best_dev = { 0 }, tmp_dev = { 0 };
    memset(dev, 0, sizeof(struct drm_device));

    for (u32 i = 0; i < vector_size(fds); i++) {
        memset(&tmp_dev, 0, sizeof(struct drm_device));
        tmp_dev.initialized_ = true;
        tmp_dev.fd = fds[i];

        /* Get the resources */
        drmModeResPtr res = drmModeGetResources(tmp_dev.fd);
        if (res == NULL) {
            destroy_drm_device(&tmp_dev);
            continue;
        }

        /* Select the connector */
        tmp_dev.conn = select_connector(tmp_dev.fd,
            res->count_connectors, res->connectors);
        if (tmp_dev.conn == NULL) {
            destroy_drm_device(&tmp_dev);
            continue;
        }

        /* Determine whether we should even proceed
         * (the current option must be the best one) */
        tmp_dev.width = tmp_dev.conn->modes[0].hdisplay;
        tmp_dev.height = tmp_dev.conn->modes[0].vdisplay;
        tmp_dev.refresh_rate = tmp_dev.conn->modes[0].vrefresh;

        u32 score = tmp_dev.width * tmp_dev.height * tmp_dev.refresh_rate;
        if (score <= top_score) {
            destroy_drm_device(&tmp_dev);
            continue;
        }

        /* Fina a suitable CRTC */
        tmp_dev.crtc = find_crtc(tmp_dev.fd, res, tmp_dev.conn);
        if (tmp_dev.crtc == NULL) {
            destroy_drm_device(&tmp_dev);
            continue;
        }

        memcpy(&curr_best_dev, &tmp_dev, sizeof(struct drm_device));
    }

    if (curr_best_dev.initialized_) {
        memcpy(dev, &curr_best_dev, sizeof(struct drm_device));
        return 0;
    } else {
        return 1;
    }
}

static drmModeConnectorPtr select_connector(i32 fd, u32 n_conns, u32 *conn_ids)
{
    drmModeConnectorPtr ret = NULL;
    u32 top_score = 0;

    for (u32 i = 0; i < n_conns; i++) {
        drmModeConnectorPtr conn = drmModeGetConnector(fd, conn_ids[i]);
        if (conn == NULL)
            continue;

        if (conn->connection != DRM_MODE_CONNECTED)
            continue;

        /* Score = Resolution * Refresh Rate */
        u32 score = conn->modes[0].hdisplay * conn->modes[0].vdisplay
            * conn->modes[0].vrefresh;
        if (score > top_score) {
            if (ret != NULL)
                drmModeFreeConnector(ret);
            ret = conn;
        } else {
            drmModeFreeConnector(conn);
        }
    }

    return ret;
}

static drmModeCrtcPtr find_crtc(i32 fd, drmModeResPtr res,
    drmModeConnectorPtr conn)
{
    drmModeEncoderPtr enc = NULL;
    drmModeCrtcPtr ret = NULL;

    /* First, see if the connector's default encoder+CRTC are OK */
    if (conn->encoder_id) {
        enc = drmModeGetEncoder(fd, conn->encoder_id);
        if (enc != NULL && enc->crtc_id) {
            ret = drmModeGetCrtc(fd, enc->crtc_id);
            if (ret != NULL) {
                drmModeFreeEncoder(enc);
                return ret;
            }
        }
    }
    if (enc != NULL) {
        drmModeFreeEncoder(enc);
        enc = NULL;
    }

    /* If no Encoder/CRTC was attached to the connector,
     * find a suitable one in it's encoder list */
    for (u32 i = 0; i < conn->count_encoders; i++) {
        enc = drmModeGetEncoder(fd, conn->encoders[i]);
        if (enc == NULL)
            continue;

        for (u32 crtc_id = 0; crtc_id < res->count_crtcs; crtc_id++) {
            if (enc->possible_crtcs & (1 << crtc_id)) {
                /* We found a compatible CRTC! Yay! */
                ret = drmModeGetCrtc(fd, crtc_id);
                if (ret != NULL) {
                    drmModeFreeEncoder(enc);
                    return ret;
                }
            }
        }

        drmModeFreeEncoder(enc);
    }

    return NULL;
}

static i32 dri_dev_scandir_filter(const struct dirent *d)
{
    return !strncmp(d->d_name, DRI_DEV_NAME_PREFIX,
        u_strlen(DRI_DEV_NAME_PREFIX));
}

static void destroy_drm_device(struct drm_device *dev)
{
    if (dev == NULL) return;

    if (dev->crtc != NULL) {
        drmModeFreeCrtc(dev->crtc);
        dev->crtc = NULL;
    }
    if (dev->conn != NULL) {
        drmModeFreeConnector(dev->conn);
        dev->conn = NULL;
    }
    if (dev->res != NULL) {
        drmModeFreeResources(dev->res);
        dev->res = NULL;
    }
    if (dev->fd != -1) {
        close(dev->fd);
        dev->fd = -1;
    }

    dev->width = dev->height = dev->refresh_rate = 0;
}
