#include "../ptime.h"
#include "core/util.h"
#include <core/int.h>
#include <core/log.h>
#include <stdatomic.h>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif /* WIN32_LEAN_AND_MEAN */
#include <windows.h>
#include <winnt.h>
#include <synchapi.h>
#include <minwindef.h>
#include <profileapi.h>
#include <sysinfoapi.h>

#define MODULE_NAME "time"

static volatile atomic_flag frequency_initialized = ATOMIC_FLAG_INIT;
static _Atomic u64 frequency;

void p_time(timestamp_t *o)
{
    u_check_params(o != NULL);

    FILETIME file_time;
    ULARGE_INTEGER total_nanoseconds;
    GetSystemTimePreciseAsFileTime(&file_time);

    total_nanoseconds.LowPart = file_time.dwLowDateTime;
    total_nanoseconds.HighPart = file_time.dwHighDateTime;

    /* Convert Windows time (seconds since Jan 1, 1601)
     * to UNIX time (seconds since Jan 1, 1970) */
    total_nanoseconds.QuadPart -= 116444736000000000LL;

    /* `FILETIME` stores time in 100-ns resolution */
    o->s = total_nanoseconds.QuadPart / 10000000LL;
    o->ns = (total_nanoseconds.QuadPart % 10000000LL) * 100;
}

void p_time_get_ticks(timestamp_t *o)
{
    u_check_params(o != NULL);

    /* We can ignore the race condition here, because
     * `QueryPerformanceFrequency` always returns the same value,
     * so `frequency` getting written to twice doesn't change anything */
    if (!atomic_flag_test_and_set(&frequency_initialized)) {
        LARGE_INTEGER tmp;
        /* This function always succeeds on Windows XP or later */
        (void) QueryPerformanceFrequency(&tmp);

        atomic_store(&frequency, tmp.QuadPart);
    }

    LARGE_INTEGER counter;
    /* This function also always succeeds on Windows XP or later */
    (void) QueryPerformanceCounter(&counter);

    /* Convert to nanoseconds based on the clock resolution (frequency) */
    u64 ns = (counter.QuadPart * 1000000000LL) / atomic_load(&frequency);

    /* Fill the timestamp struct */
    o->s = ns / 1000000000LL;
    o->ns = ns % 1000000000LL;
}

i64 p_time_delta_us(const timestamp_t *t0)
{
    if (t0 == NULL) return 0;

    timestamp_t o = { 0 }, t1 = { 0 };
    p_time_get_ticks(&t1);
    timestamp_delta(o, *t0, t1);

    return (o.s * 1000000) + (o.ns / 1000);
}

i64 p_time_delta_ms(const timestamp_t *t0)
{
    if (t0 == NULL) return 0;

    timestamp_t o = { 0 }, t1 = { 0 };
    p_time_get_ticks(&t1);
    timestamp_delta(o, *t0, t1);

    return (o.s * 1000) + (o.ns / 1000000);
}

i64 p_time_delta_s(const timestamp_t *t0)
{
    if (t0 == NULL) return 0;

    timestamp_t o = { 0 }, t1 = { 0 };
    p_time_get_ticks(&t1);
    timestamp_delta(o, *t0, t1);

    return o.s + o.ns / 1000000000;
}

void p_time_nanosleep(const timestamp_t *time)
{
    if (time == NULL) return;
    Sleep((time->s * 1000) + (time->ns / 1000000));
}

void p_time_usleep(u32 u_seconds)
{
    Sleep(u_seconds / 1000);
}

void p_time_msleep(u32 m_seconds)
{
    Sleep(m_seconds);
}

void p_time_sleep(u32 seconds)
{
    Sleep(seconds * 1000);
}
