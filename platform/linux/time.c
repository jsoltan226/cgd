#define _POSIX_C_SOURCE 199309L
#include "../time.h"
#include "core/int.h"
#include "core/log.h"
#include <errno.h>
#include <stdlib.h>
#include <linux/time.h>
#include <string.h>

#define MODULE_NAME "time"

/* clangd is really stupid ngl */
int clock_gettime (__clockid_t __clock_id, struct timespec *__tp);

i32 p_time(p_time_t *o)
{
    if (o == NULL) {
        s_log_error("Invalid parameters");
        return -1;
    }

    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts)) {
        s_log_error("Failed to get current time: %s", strerror(errno));
        return 1;
    }

    o->ns = ts.tv_nsec;
    o->us = o->ns / 1000;
    o->ms = o->us / 1000;
    o->s = ts.tv_sec;

    return 0;
}

i32 p_time_since(p_time_t *o, p_time_t *since)
{
    if (o == NULL || since == NULL) {
        s_log_error("Invalid parameters");
        return -1;
    }

    if (p_time(o)) return 1;

    o->ns -= since->ns;
    o->us -= since->us;
    o->ms -= since->ms;
    o->s -= since->s;

    return 0;
}