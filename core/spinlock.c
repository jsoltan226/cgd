#include "spinlock.h"
#include <stdatomic.h>

void spinlock_acquire(spinlock_t *lock)
{
    while (atomic_flag_test_and_set_explicit(lock, memory_order_acquire)) {
        /* Keep the CPU busy with some useless work for a little while
         * to hint to the core that it can reschedule itself for use
         * by another thread */
        for (volatile u64 i = 0; i < 1024; i++)
            ;
    }
}

void spinlock_release(spinlock_t *lock)
{
    atomic_flag_clear(lock);
}
