#include "../exe-info.h"
#include <core/int.h>
#include <core/log.h>
#include <stdio.h>
#include <windows.h>
#include <libloaderapi.h>
#include <errhandlingapi.h>

#define MODULE_NAME "exe-info"

i32 p_get_exe_path(char *buf, u32 buf_size)
{
    if (!GetModuleFileName(NULL, buf, buf_size - 1)) {
        s_log_error("GetModuleFileName() failed. Reason: %s.\n",
            GetLastError());
    }
    buf[buf_size - 1] = '\0';
    return 0;
}
