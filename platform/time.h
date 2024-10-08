#ifndef P_TIME_H_
#define P_TIME_H_

#include <core/int.h>

typedef struct {
    i64 ns;
    i64 us;
    i64 ms;
    i64 s;
} p_time_t;

struct p_timer {
    p_time_t start_time;
};

i32 p_time_since(p_time_t *o, p_time_t *since);
i32 p_time(p_time_t *o);

void p_time_nanosleep(p_time_t *time);
void p_time_usleep(u32 u_seconds);
void p_time_sleep(u32 seconds);

#endif /* P_TIME_H_ */
