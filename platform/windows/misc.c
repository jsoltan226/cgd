#include "../misc.h"
#include <core/int.h>
#include <core/log.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif /* WIN32_LEAN_AND_MEAN */
#include <windows.h>
#include <libloaderapi.h>
#define P_INTERNAL_GUARD__
#include "error.h"
#undef P_INTERNAL_GUARD__

#define MODULE_NAME "exe-info"

i32 p_get_exe_path(char *buf, u32 buf_size)
{
    if (!GetModuleFileName(NULL, buf, buf_size - 1)) {
        s_log_error("GetModuleFileName() failed. Reason: %s.\n",
            get_last_error_msg());
    }
    buf[buf_size - 1] = '\0';

    /* Convert DOS path to UNIX path */
    char *chr_p = NULL;
    while (chr_p = strchr(buf, '\\'), chr_p != NULL)
        *chr_p = '/';

    return 0;
}
