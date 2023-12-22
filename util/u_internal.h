#ifndef _U_INTERNAL_H
#define _U_INTERNAL_H

#include "util.h"

#define u_PATH_FROM_BIN_TO_ASSETS "../assets"

static struct {
    char binDirBuffer[u_BUF_SIZE];
    char assetsDirBuffer[u_BUF_SIZE];
} _cgd_util_internal;

#endif /* _U_INTERNAL_H */
