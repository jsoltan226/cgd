#include "asset.h"
#include "io-PNG.h"
#include "plugin.h"
#include "img-type.h"
#include <core/int.h>
#include <core/log.h>
#include <core/util.h>
#include <core/pixel.h>
#include <render/rctx.h>
#include <render/surface.h>
#include <platform/window.h>
#include <platform/exe-info.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdatomic.h>

#define MODULE_NAME "assetld"

/* No need for a mutex here since
 * we're only incrementing and decrementing here.
 * But I still made it atomic just to be safe. */
static _Atomic i32 g_n_active_handles = 0;

#define add_n_active_handles(diff) do {                         \
    const i32 current_val = atomic_load(&g_n_active_handles);   \
    atomic_store(&g_n_active_handles, current_val + diff);      \
} while (0)

const char * asset_get_assets_dir();
static i32 get_bin_dir(char *buf, u32 buf_size);

struct asset * asset_load(filepath_t rel_file_path, struct r_ctx *rctx)
{
    u_check_params(rel_file_path != NULL && rctx != NULL);

    s_log_debug("Loading asset \"%s\"...", rel_file_path);

    FILE *fp = NULL;

    struct asset *a = calloc(1, sizeof(struct asset));
    s_assert(a != NULL, "calloc() failed for %s", "struct asset");

    strncpy((char *)a->rel_file_path, rel_file_path, u_FILEPATH_MAX - 1);
    fp = asset_fopen(rel_file_path, "rb");
    if (fp == NULL)
        goto_error("Failed to open asset \"%s\": %s",
            a->rel_file_path, strerror(errno));

    a->type = asset_get_img_type(fp);
    if (a->type == IMG_TYPE_UNKNOWN)
        goto_error("Asset image type is UNKNOWN!");

    i32 plugin_loaded = asset_get_plugin_loaded(a->type);
    if (plugin_loaded == PLUGIN_DOES_NOT_EXIST) {
        goto_error("Plugin for type \"%s\" does not exist!",
            asset_get_type_name(a->type));
    } else if(plugin_loaded == PLUGIN_NOT_LOADED) {
        if (asset_load_plugin_by_type(a->type))
            goto_error("Failed to load plugin for type \"%s\"");
    }

    switch(a->type) {
        case IMG_TYPE_PNG:
            if (read_PNG(&a->pixel_data, fp))
                goto_error("read_PNG Failed for \"%s\"", a->rel_file_path);
            break;
        default: case IMG_TYPE_UNKNOWN:
            goto err;
            break;
    }

    a->surface = r_surface_init(rctx, &a->pixel_data, P_WINDOW_RGBA8888);
    if (a->surface == NULL)
        goto_error("Failed to create a surface from the pixel_data!");

    fclose(fp);
    add_n_active_handles(1);
    return a;

err:
    if (fp) fclose(fp);
    if (a) asset_destroy(&a);
    return NULL;
}

FILE * asset_fopen(const char *rel_file_path, const char *mode)
{
    if (rel_file_path == NULL || mode == NULL) return NULL;

    FILE *fp = NULL;
    char full_path_buf[u_BUF_SIZE] = { 0 };

    strncpy(full_path_buf, asset_get_assets_dir(), u_BUF_SIZE - 1);
    strncat(full_path_buf, rel_file_path, u_BUF_SIZE - strlen(full_path_buf) - 1);

    fp = fopen(full_path_buf, mode);
    return fp; /* NULL will be returned if fp is NULL */
}

void asset_destroy(struct asset **asset)
{
    if (asset == NULL || *asset == NULL) return;

    struct asset *a = *asset;

    if (a->surface)
        r_surface_destroy(&a->surface);
    else if (a->pixel_data.buf)
        free(a->pixel_data.buf);

    u_nzfree(&a);

    add_n_active_handles(-1);
    const i32 curr_n_active_handles = atomic_load(&g_n_active_handles);

    if (curr_n_active_handles == 0)
        asset_unload_all_plugins();
    else if (curr_n_active_handles < 0)
        s_log_error("%s(): Invalid value of g_n_active_handles: %i",
            __func__, curr_n_active_handles);
}

static char bin_dir_buf[u_BUF_SIZE] = { 0 };
static char asset_dir_buf[u_BUF_SIZE] = { 0 };

const char * asset_get_assets_dir()
{
    if (bin_dir_buf[0] != '/') {
        if (get_bin_dir(bin_dir_buf, u_BUF_SIZE)) {
            s_log_error("[%s] Failed to get the bin dir", __func__);
            return NULL;
        }
    }

    if (asset_dir_buf[0] != '/') {
        strncpy(asset_dir_buf, bin_dir_buf, u_BUF_SIZE);
        asset_dir_buf[u_BUF_SIZE - 1] = '\0';
        strncat(
            asset_dir_buf,
            u_PATH_FROM_BIN_TO_ASSETS,
            u_BUF_SIZE - strlen(asset_dir_buf) - 1
        );

        asset_dir_buf[u_BUF_SIZE - 1] = '\0';
        s_log_debug("%s: The asset dir is \"%s\"", __func__, asset_dir_buf);
    }

    return asset_dir_buf;
}

static i32 get_bin_dir(char *buf, u32 buf_size)
{
    u_check_params(buf != NULL && buf_size > 0);

    memset(buf, 0, buf_size);
    if (p_get_exe_path(buf, buf_size)) {
        s_log_error("%s: Failed to get the path to the executable.\n", __func__);
        return 1;
    }

    /* Cut off the string after the last '/' */
    u32 i = buf_size - 1;
    while (buf[i] != '/' && i-- >= 0);
    buf[i + 1] = '\0';

    return 0;

}

#undef MODULE_NAME
#define MODULE_NAME "assetwr"

i32 asset_write(struct asset *a, const char *rel_file_path, enum asset_img_type img_type)
{
    u_check_params(a != NULL);

    if (rel_file_path == NULL)
        rel_file_path = a->rel_file_path;
    if (img_type == IMG_TYPE_UNKNOWN)
        img_type = a->type;

    s_log_debug("Writing asset \"%s\" to path \"%s\"", a->rel_file_path, rel_file_path);

    FILE *fp = asset_fopen(rel_file_path, "wb");
    if (fp == NULL) {
        s_log_error("Failed to open out file \"%s\": %s", rel_file_path, strerror(errno));
        return -2;
    }

    switch(a->type) {
        case IMG_TYPE_PNG:
            if (write_PNG(&a->pixel_data, fp)) {
                s_log_error("write_PNG failed for \"%s\"", a->rel_file_path);
                fclose(fp);
                return -3;
            }
        case IMG_TYPE_UNKNOWN: default:
            s_log_error("image type of \"%s\" is UNKNOWN!", a->rel_file_path);
            fclose(fp);
            return -4;
    }

    fclose(fp);

    return EXIT_SUCCESS;
}
