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
    if (clock_gettime(CLOCK_MONOTONIC, &ts)) {
        s_log_error("Failed to get current time: %s", strerror(errno));
        return 1;
    }

    /* Let's suppose that each letter represents a digit:
     *      seconds are 's',
     *      milliseconds are 'M',
     *      microseconds are 'U',
     *      nanoseconds are 'N'.
     *
     * `clock_gettime` gives us the time in the format:
     *      ssssssssss.NNNNNNNNNN,
     *  with `tv_sec` representing the 's' seconds,
     *  and `tv_nsec` being the 'N' nanoseconds.
     *  
     *  We, however, need to return the time in this format:
     *      ssssssssss.MMM.UUU.NNN
     *                                                                      */ 
    /* So we can just keep the seconds from `tv_sec`                        */
    o->s = ts.tv_sec;

    /* Note that if the time in `tv_nsec` was stored in binary
     * (each letter in "MMM.UUU.SSS" is a bit instead of a decimal digit)
     * this would just be done with bitwise operations;
     * for `MMM`, you would:
     *      1. right-shift by 6 to get the number,
     *      2. AND with 000.111.111 to "delete" it.
     *
     * for `UUU` you would do the same thing, but instead of
     * shifting by 6 you would shift by 3,
     * and the deletion mask would be 000.000.111 .
     * 
     * After that you would just be left with 000.000.NNN,
     * which is exactly what happens here at the end.
     *
     * What we are doing here is the exact thing I described above,
     * but we deal with base 10 instead of base 2.
     */

     /* we start here: MMM.UUU.NNN
     *                 ^^^
     *
     * Right-shift the digits by 6, AKA divide by 10^6                      */
    o->ms = ts.tv_nsec / 1000000;

    /* We then "delete" those 3 digits by taking
     * only the remainder of the previous division,
     * so we are effectively left with this:
     *      000.UUU.NNN
     *      ^^^                                                             */
    ts.tv_nsec = ts.tv_nsec % 1000000;

    /* Now we repeat the same process for microseconds,
     * which are stored 
     * (here): 000.UUU.NNN
     *             ^^^
     *
     * and so we operate on 10^3 instead of 10^6. */
    o->us = ts.tv_nsec / 1000;
    ts.tv_nsec = ts.tv_nsec % 1000;

    /* Now, as said in the beginning,
     * we are left with only the nanoseconds:
     *      000.000.NNN
     * and so we just copy them over. */
    o->ns = ts.tv_nsec;


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
