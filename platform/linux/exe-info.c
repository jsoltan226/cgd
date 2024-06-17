#include "../exe-info.h"
#include "core/int.h"
#include <unistd.h>
#include <stdio.h>

i32 p_get_exe_path(char *buf, u32 buf_size)
{
    if (!readlink("/proc/self/exe", buf, buf_size - 1)) {
        fprintf(stderr, "[getExePath]: ERROR: Readlink for /proc/self/exe failed.\n");
        return 1;
    }
    buf[buf_size] = '\0';
    return 0;
}
