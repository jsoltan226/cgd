#include "util.h"
#include "int.h"
#include "log.h"
#include "platform/exe-info.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static char bin_dir_buf[u_BUF_SIZE] = { 0 };
static char asset_dir_buf[u_BUF_SIZE] = { 0 };

const char *u_get_asset_dir()
{
    if (bin_dir_buf[0] != '/') {
        if (u_get_bin_dir(bin_dir_buf, u_BUF_SIZE)) {
            s_log_error("util", "[u_get_asset_dir] Failed to get the bin dir");
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
        s_log_debug("util", "[u_get_asset_dir] The asset dir is \"%s\"", asset_dir_buf);
    }

    return asset_dir_buf;
}

i32 u_get_bin_dir(char *buf, u32 buf_size)
{
    if (buf == NULL) {
        s_log_error("util", "[u_get_bin_dir] The provided buffer is NULL");
        return 1;
    } else if (buf_size == 0) {
        s_log_error("util", "[u_get_bin_dir] The provided buffer size is 0\n");
        return 1;
    }

    memset(buf, 0, buf_size);
    if (p_get_exe_path(buf, buf_size)) {
        s_log_error("util", "[u_get_bin_dir] Failed to get the path to the executable.\n");
    }

    /* Cut off the string after the last '/' */
    u32 i = buf_size - 1;
    while (buf[i] != '/' && i-- >= 0);
    buf[i + 1] = '\0';
    s_log_debug("util", "[u_get_bin_dir] The bin dir is \"%s\"", buf);

    return 0;
}

bool u_collision(const rect_t *r1, const rect_t *r2)
{
    return (
        r1->x <= r2->x + r2->w &&
        r1->x + r1->w >= r2->x &&
        r1->y <= r2->y + r2->h &&
        r1->y + r1->h >= r2->y
    );
}
