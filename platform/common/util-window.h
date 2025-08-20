#ifndef PLATFORM_UTIL_WINDOW_H_
#define PLATFORM_UTIL_WINDOW_H_

#include "guard.h"
#include <core/int.h>
#include <platform/window.h>

i32 pc_window_check_flags(u32 flags);

void pc_window_set_default_flags(u32 *flags);

typedef void * pc_window_handle_t;
typedef i32 (*pc_window_set_acceleration_fn_t)
    (pc_window_handle_t, enum p_window_acceleration);

i32 pc_window_initialize_acceleration_from_flags(
    pc_window_set_acceleration_fn_t set_acceleration_fn,
    pc_window_handle_t win, u32 flags
);

#endif /* PLATFORM_UTIL_WINDOW_H_ */
