## NEWS for 25.11.2024
* Minor changes and bug fixes
    * Adapted the whole thread subsystem to `p_mt_thread_wait` returning `void`
    * Removed the `flags` field from `p_mt_thread_create`
    * Deleted the now useless declarations of `window_X11_attach_fb` and `window_X11_detach_fb`
    * Finally fully fixed the async-signal safety issues in `platform/linux/event`

## NEWS for 07.12.2024
* Reworked the `platform/time` API, fixed some bugs
    * Fixed a potential `NULL` pointer derefernce in `asset_fopen` (in `asset-loader/asset.c`)
    * `asset_dir_buf` and `bin_dir_buf` in `asset-loader/asset.c` are now guarded by mutexes
    * Removed the (should-have-been-temporary) condition that allowed for `evdev_find_and_load_devices`
        (from `platform/linux/evdev`) to return success when looking for `MOUSE_EVDEV` devices,
        even when none were actually found
    * Reworked the `platform/time` API:
        * The `p_time_t` struct got replaced by `timestamp_t`, which only contains fields
            for seconds and nanoseconds, similar to the linux `timespec` struct
        * `p_time_since` got removed
        * `p_time_get_ticks` now handles the original functionality of `p_time` (high-precision timers)
        * `p_time` will now give imprecise but UTC-synced time, similar to the C stdlib `time` function
    * Added `tests/time-test`
    * Added `tests/log-util.h`, which provides `test_log_setup`.
        So now all that log init and cleanup code is no longer being duplicated across
        all the tests' main functions, improving their simplicity and readability.
        Yay!
    * Fixed `TEST_LIB` being rebuilt each time the contents of `TEST_BINDIR` were changed
    * Renamed `platform/exe-info` to `platform/misc`
