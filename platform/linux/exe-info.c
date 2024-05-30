#include "../exe-info.h"
#include <unistd.h>
#include <stdio.h>

int p_getExePath(char *buf, size_t buf_size)
{
    if (!readlink("/proc/self/exe", buf, buf_size - 1)) {
        fprintf(stderr, "[getExePath]: ERROR: Readlink for /proc/self/exe failed.\n");
        return 1;
    }
    buf[buf_size] = '\0';
    return 0;
}
