#include "../time.h"
#include <core/int.h>
#include <core/log.h>
#include <stdlib.h>
#include <stdatomic.h>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif /* WIN32_LEAN_AND_MEAN */
#include <windows.h>
#include <winnt.h>
#include <synchapi.h>
#include <minwindef.h>
#include <profileapi.h>

#define MODULE_NAME "time"

static volatile atomic_flag frequency_initialized = ATOMIC_FLAG_INIT;
static _Atomic LARGE_INTEGER frequency;

i32 p_time(p_time_t *o)
{
    if (o == NULL) {
        s_log_error("Invalid parameters");
        return -1;
    }

    /* We can ignore the race condition here, because
     * `QueryPerformanceFrequency` always returns the same value,
     * so `frequency` getting written to twice doesn't change anything */
    if (!atomic_flag_test_and_set(&frequency_initialized)) {
        LARGE_INTEGER tmp;
        /* This function always succeeds on Windows XP or later */
        (void) QueryPerformanceFrequency(&tmp);

        atomic_store(&frequency, tmp);
    }

    LARGE_INTEGER counter;
    /* This function also always succeeds on Windows XP or later */
    (void) QueryPerformanceCounter(&counter);

    /* Convert to nanoseconds based on the frequency */
    u64 ns = (counter.QuadPart * 1000000000LL) /
        atomic_load(&frequency).QuadPart;

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

i32 p_time_since(p_time_t *o, const p_time_t *since)
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
