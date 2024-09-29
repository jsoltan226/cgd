#include "../exe-info.h"
#include <core/int.h>
#include <stdio.h>
#include <windows.h>

i32 p_get_exe_path(char *buf, u32 buf_size)
{
    if (!GetModuleFileName(NULL, buf, buf_size - 1)) {
        fprintf(stderr, "GetModuleFileName() failed. Reason: %s.\n", GetLastError());
    }
    buf[buf_size - 1] = '\0';
    return 0;
}
