#include <core/int.h>
#include <core/log.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif /* WIN32_LEAN_AND_MEAN */
#include <windows.h>
#include <winnt.h>
#include <errhandlingapi.h>
#define P_INTERNAL_GUARD__
#include "error.h"
#undef P_INTERNAL_GUARD__

#define MODULE_NAME "error"

#define G_ERRORMSG_BUF_SIZE 1024
static char g_errormsg_buf[G_ERRORMSG_BUF_SIZE] = { 0 };

const char * get_last_error_msg(void)
{
    DWORD msg_id = GetLastError();
    if (msg_id == 0)
        return NULL;

    u64 size = FormatMessage(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, msg_id, LANG_ENGLISH,
        g_errormsg_buf, G_ERRORMSG_BUF_SIZE - 1, NULL
    );
    if (size == 0) {
        snprintf(g_errormsg_buf, G_ERRORMSG_BUF_SIZE - 1,
            "Unknown error: %lu", msg_id);
    }

    /* Delete the newline character at the end */
    char *chr_p = strrchr(g_errormsg_buf, '\r');
    if (chr_p != NULL) *chr_p = '\0';

    return g_errormsg_buf;
}
