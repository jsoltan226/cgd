#include "util.h"
#include "u_internal.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <cgd/platform/exe-info.h>

const char *u_getAssetPath()
{
    if (_cgd_util_internal.binDirBuffer[0] != '/') {
        if (u_getBinDir(_cgd_util_internal.binDirBuffer, u_BUF_SIZE)) return NULL;
    }

    if (_cgd_util_internal.assetsDirBuffer[0] != '/') {
        if (strncpy( _cgd_util_internal.assetsDirBuffer, _cgd_util_internal.binDirBuffer, u_BUF_SIZE) == NULL)
            return NULL;
        _cgd_util_internal.assetsDirBuffer[u_BUF_SIZE - 1] = '\0';

        if (strncat(
                _cgd_util_internal.assetsDirBuffer,
                u_PATH_FROM_BIN_TO_ASSETS, 
                u_BUF_SIZE - strlen(_cgd_util_internal.assetsDirBuffer) - 1
        ) == NULL)
            return NULL;
        _cgd_util_internal.assetsDirBuffer[u_BUF_SIZE - 1] = '\0';
    }

    return _cgd_util_internal.assetsDirBuffer;
}

/* You already know what this does */
int u_max(int a, int b)
{
    if(a > b)
        return a;
    else 
        return b;
}

/* Same here */
int u_min(int a, int b)
{
    if(a < b)
        return a;
    else 
        return b;
}

/* The simplest collision checking implementation;
 * returns true if 2 rectangles overlap 
 */
bool u_collision(const SDL_Rect *r1, const SDL_Rect *r2)
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

int u_getBinDir(char *buf, size_t buf_size)
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
    int i = buf_size - 1;
    while (buf[--i] != '/' && i >= 0);
    buf[i + 1] = '\0';

    return 0;
}
