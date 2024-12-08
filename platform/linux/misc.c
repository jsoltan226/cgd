#define _GNU_SOURCE
#include "../misc.h"
#include <core/int.h>
#include <core/log.h>
#include <core/util.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#define SELF_EXE_FILE "/proc/self/exe"
#define MODULE_NAME "exe-info"

i32 p_get_exe_path(char *buf, u32 buf_size)
{
    u_check_params(buf != NULL && buf_size > 0);

    buf[buf_size - 1] = '\0';
    i32 n_bytes_read = readlink(SELF_EXE_FILE, buf, buf_size - 1);
    if (n_bytes_read == -1) {
        s_log_error("%s: readlink() for %s failed: %s\n",
            __func__, SELF_EXE_FILE, strerror(errno));
        return 1;
    } else if (n_bytes_read == buf_size - 1) {
        s_log_error(
            "%s: The output of readlink() was truncated (output buffer size %u was too small)",
            __func__, buf_size
        );
        return 2;
    }
    return 0;
}
