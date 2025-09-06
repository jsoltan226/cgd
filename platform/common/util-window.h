#ifndef PLATFORM_UTIL_WINDOW_H_
#define PLATFORM_UTIL_WINDOW_H_

#include "guard.h"
#include <core/int.h>
#include <platform/window.h>

/* Checks that the `flags` are valid and don't contradict each other,
 * as per `platform/window.h` */
i32 pc_window_check_flags(u32 flags);

/* Sets the defaults for the values in `*flags` that weren't set.
 * Note: You must first validate the flags with `pc_window_check_flags`. */
void pc_window_set_default_flags(u32 *flags);

/* A substitute for `struct p_window *`
 * in `pc_window_initialize_acceleration_from_flags` */
typedef void * pc_window_handle_t;

/* The `p_window_set_acceleration` function pointer
 * for `pc_window_initialize_acceleration_from_flags` */
typedef i32 (*pc_window_set_acceleration_fn_t)
    (pc_window_handle_t, enum p_window_acceleration);

/* Initializes the acceleration of `win` with the function `set_acceleration_fn`
 * choosing which mode to initialize and/or fall back to based on `flags`.
 *
 * `set_acceleration_fn` should be a `p_window_set_acceleration`-like function,
 * except that its `win` parameter should be a generic `void *`.
 * This is done because implementations often call their own internal
 * version of `p_window_set_acceleration` on their own internal `win` struct.
 *
 * Notes:
 *  - `flags` must have been previously validated with `pc_window_check_flags`.
 *  - The window's acceleration mode must be explicitly initialized to
 *      `P_WINDOW_ACCELERATION_UNSET_` before calling this function.
 */
i32 pc_window_initialize_acceleration_from_flags(
    pc_window_set_acceleration_fn_t set_acceleration_fn,
    pc_window_handle_t win, u32 flags
);

#endif /* PLATFORM_UTIL_WINDOW_H_ */
