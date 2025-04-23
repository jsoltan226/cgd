#include "spinlock.h"
#include <stdatomic.h>

void spinlock_acquire(spinlock_t *lock)
{
    while (atomic_flag_test_and_set_explicit(lock, memory_order_acquire))
        ;
}

void spinlock_release(spinlock_t *lock)
{
    atomic_flag_clear(lock);
}
