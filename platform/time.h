#ifndef P_TIME_H_
#define P_TIME_H_

#include <core/int.h>

typedef struct {
    i64 ns;
    i64 us;
    i64 ms;
    i64 s;
} p_time_t;

i32 p_time(p_time_t *o);

i32 p_time_since(p_time_t *o, const p_time_t * restrict since);
i64 p_time_delta_us(const p_time_t *t0);
i64 p_time_delta_ms(const p_time_t *t0);
i64 p_time_delta_s(const p_time_t *t0);

void p_time_nanosleep(const p_time_t *time);
void p_time_usleep(u32 u_seconds);
void p_time_msleep(u32 m_seconds);
void p_time_sleep(u32 seconds);

#endif /* P_TIME_H_ */
