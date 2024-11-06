#ifndef P_TIME_H_
#define P_TIME_H_

#include <core/int.h>

/* Note that this whole abstraction layer is intended for
 * measuring time INTERVALS, not creating timestamps. */

typedef struct {
    i64 ns; /* Nano secdons */
    i64 us; /* Micro seconds (u-seconds) */
    i64 ms; /* Milli seconds */
    i64 s;  /* Seconds */
} p_time_t;

/* Retrieves the current time into `o`.
 * Returns 0 on success and non-zero on failure. */
i32 p_time(p_time_t *o);

/* Retrieves the difference of `since` and the current time into `o`.
 * Returns 0 on success and non-zero on failure */
i32 p_time_since(p_time_t *o, const p_time_t *since);

/* Self-explanatory */
i64 p_time_delta_us(const p_time_t *t0);
i64 p_time_delta_ms(const p_time_t *t0);
i64 p_time_delta_s(const p_time_t *t0);

/* Also self-explanatory */
void p_time_nanosleep(const p_time_t *time);
void p_time_usleep(u32 u_seconds);
void p_time_msleep(u32 m_seconds);
void p_time_sleep(u32 seconds);

#endif /* P_TIME_H_ */
