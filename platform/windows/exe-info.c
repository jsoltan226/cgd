#include "../exe-info.h"

#include <stddef.h>
#include <stdio.h>
#include <windows.h>

int p_getExePath(char *buf, size_t buf_size)
{
    if (!GetModuleFileName(NULL, buf, buf_size - 1)) {
        fprintf(stderr, "GetModuleFileName() failed. Reason: %s.\n", GetLastError());
    }
    buf[buf_size - 1] = '\0';
    return 0;
}
