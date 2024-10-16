#define _POSIX_C_SOURCE 199309L
#include "../time.h"
#include <core/int.h>
#include <core/log.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <linux/time.h>

#define MODULE_NAME "time"

/* clangd is really stupid ngl */
extern int clock_gettime (__clockid_t __clock_id, struct timespec *__tp);
extern int clock_nanosleep(__clockid_t __clock_id, int flags,
    const struct timespec *rqtp, struct timespec *rmtp);

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

void p_time_nanosleep(p_time_t *time)
{
    u64 ns_sum = time->ns + (time->us * 1000) + (time->ms * 1000000);

    /* Handle potential overflows */
    u64 s_sum = time->s + (ns_sum / 1000000000);
    ns_sum = ns_sum % 1000000000;

    struct timespec ts_req = {
        .tv_sec = s_sum,
        .tv_nsec = ns_sum,
    };
    clock_nanosleep(CLOCK_REALTIME, 0, &ts_req, NULL);
}

void p_time_usleep(u32 u_seconds)
{
    p_time_nanosleep(&(p_time_t) { .us = u_seconds });
}

void p_time_msleep(u32 m_seconds)
{
    p_time_nanosleep(&(p_time_t) { .ms = m_seconds });
}

void p_time_sleep(u32 seconds)
{
    p_time_nanosleep(&(p_time_t) { .s = seconds });
}
