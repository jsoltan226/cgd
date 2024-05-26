#include "util.h"
#include "int.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <cgd/platform/exe-info.h>

static char bin_dir_buf[u_BUF_SIZE] = { 0 };
static char asset_dir_buf[u_BUF_SIZE] = { 0 };

const char *u_get_asset_dir()
{
    if (bin_dir_buf[0] != '/') {
        if (u_get_bin_dir(bin_dir_buf, u_BUF_SIZE)) return NULL;
    }

    if (asset_dir_buf[0] != '/') {
        if (strncpy( asset_dir_buf, bin_dir_buf, u_BUF_SIZE) == NULL)
            return NULL;
        asset_dir_buf[u_BUF_SIZE - 1] = '\0';

        if (
            strncat(
                asset_dir_buf,
                u_PATH_FROM_BIN_TO_ASSETS, 
                u_BUF_SIZE - strlen(asset_dir_buf) - 1
            ) == NULL
        ) return NULL;

        asset_dir_buf[u_BUF_SIZE - 1] = '\0';
    }

    return asset_dir_buf;
}

/* You already know what this does */
i64 u_max(i64 a, i64 b)
{
    if(a > b)
        return a;
    else 
        return b;
}

/* Same here */
i64 u_min(i64 a, i64 b)
{
    if(a < b)
        return a;
    else 
        return b;
}

/* The simplest collision checking implementation;
 * returns true if 2 rectangles overlap 
 */
bool u_collision(const rect_t *r1, const rect_t *r2)
{
    return (
            r1->x + r1->w >= r2->x &&
            r1->x <= r2->x + r2->w &&
            r1->y + r1->h >= r2->y &&
            r1->y <= r2->y + r2->h 
           );
}

void u_error(const char *fmt, ...)
{
    va_list vaList;
    va_start(vaList, fmt);
    vfprintf(stderr, fmt, vaList);
    va_end(vaList);
}

i32 u_get_bin_dir(char *buf, u32 buf_size)
{
    if (buf == NULL) {
        fprintf(stderr, "[getBinDir]: ERROR: The provided string is a null pointer.\n");
        return 1;
    } else if (buf_size == 0) {
        fprintf(stderr, "[u_getBinDir] ERROR: The provided buffer size is 0\n");
        return 1;
    }

    memset(buf, 0, buf_size);
    if (p_getExePath(buf, buf_size)) {
        fprintf(stderr, "[u_getBinDir] Failed to get the path to the executable.\n");
    }

    /* Cut off the string after the last '/' */
    u32 i = buf_size - 1;
    while (buf[--i] != '/' && i >= 0);
    buf[i + 1] = '\0';

    return 0;
}
