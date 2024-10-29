#include "../time.h"
#include <core/int.h>
#include <core/log.h>
#include <stdlib.h>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif /* WIN32_LEAN_AND_MEAN */
#include <windows.h>
#include <winnt.h>
#include <synchapi.h>
#include <minwindef.h>
#include <sysinfoapi.h>

#define MODULE_NAME "time"

/* clangd is really stupid ngl */
i32 p_time(p_time_t *o)
{
    if (o == NULL) {
        s_log_error("Invalid parameters");
        return -1;
    }

    FILETIME ft;
    LARGE_INTEGER li;

    GetSystemTimePreciseAsFileTime(&ft);
    li.LowPart = ft.dwLowDateTime;
    li.HighPart = ft.dwHighDateTime;

    /* Convert to nanoseconds since Jan 1, 1601 */
    i64 ns = (li.QuadPart - 116444736000000000LL) / 10;

    /* Fill the time struct */
    /* See `platform/linux/time.c` for more detailed explanations */
    o->s = ns / 1000000000;
    ns %= 1000000000;
    o->ms = ns / 1000000;
    ns %= 1000000;
    o->us = ns / 1000;
    o->ns = ns % 1000;

    return 0;
}

i32 p_time_since(p_time_t *o, const p_time_t * restrict since)
{
    if (o == NULL || since == NULL) {
        s_log_error("Invalid parameters");
        return -1;
    }

    if (p_time(o)) return 1;

    i64 carry = 0, total = 0;

    if (since->ns > o->ns) {
        o->ns -= 1000 - since->ns;
        carry = -since->ns / 1000;
    } else {
        o->ns -= since->ns;
    }

    total = since->us + carry;
    carry = 0;
    if (total > o->us) {
        o->us -= 1000 - since->us;
        carry = -since->us / 1000;
    } else {
        o->us -= total;
    }

    total = since->ms + carry;
    carry = 0;
    if (total > o->ms) {
        o->ms -= 1000 - since->ms;
        carry = -since->ms / 1000;
    } else {
        o->ms -= total;
    }

    total = since->s + carry;
    o->s -= total;

    return 0;
}

i64 p_time_delta_us(const p_time_t *t0)
{
    if (t0 == NULL) return 0;

    p_time_t t1 = { 0 };
    (void) p_time_since(&t1, t0);
    
    return t1.s * 1000000 + t1.ms * 1000 + t1.us;
}

i64 p_time_delta_ms(const p_time_t *t0)
{
    if (t0 == NULL) return 0;

    p_time_t t1 = { 0 };
    (void) p_time_since(&t1, t0);
    
    return t1.s * 1000 + t1.ms;
}

i64 p_time_delta_s(const p_time_t *t0)
{
    if (t0 == NULL) return 0;

    p_time_t t1 = { 0 };
    (void) p_time_since(&t1, t0);

    return t1.s;
}

void p_time_nanosleep(const p_time_t *time)
{
    const u64 total_ms = (time->ns / 1000000)
        + (time->us / 1000)
        + (time->ms)
        + (time->s * 1000);

    Sleep(total_ms);
}

void p_time_usleep(u32 u_seconds)
{
    p_time_nanosleep(&(const p_time_t) { .us = u_seconds });
}

void p_time_msleep(u32 m_seconds)
{
    p_time_nanosleep(&(const p_time_t) { .ms = m_seconds });
}

void p_time_sleep(u32 seconds)
{
    p_time_nanosleep(&(const p_time_t) { .s = seconds });
}
