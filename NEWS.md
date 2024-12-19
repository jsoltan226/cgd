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

## NEWS for 18.12.2024
* Made `r_surface` more independent of `r_ctx`, changed some outdated naming
    * `struct r_surface` no longer contains `rctx`
    * `r_surface_blit` will now blit from one surface to another,
        it's previous functionality got replaced by `r_surface_render`
    * Since `r_surface_init` no longer requires the `rctx` argument,
        therefore the following subsystems also don't need `rctx`
        to be passed to their respective init/create functions:
        * `asset-loader/asset`
        * `gui/sprite`
        * `gui/parallax-bg`
        * `gui/button`
        * `gui/menu`
        * `gui/menu-mgr`
        Also, `asset-load-test` (finally) no longer uses any window or rendering context at all

    * Renamed `struct main_ctx` to `struct platform_ctx`,
        and `do_early_init`/`do_main_cleanup` to `do_platform_init`/`do_platform_cleanup`
    * Moved the rendering context (`r`) and `paused` from the `platform_ctx` to `gui_ctx`
    * `r_putpixel_rgba` now uses `r_putpixel_fast_matching_pixelfmt_`
    * Replaced the `enum p_window_color_type` with `enum pixelfmt` (`pixelfmt_t`) in `core/pixel.h`
        and shorted the names from e.g. `P_WINDOW_COLOR_RGBA8888` to `RGBA32`.
        Also added new color formats (`RGB24`, `BGR24`, `RGBX32`, `BGRX32`)
        and alternative names for them (e.g. `RGB24` -> `RGB888` and `BGRA32` -> `BGRA8888`)
    * Added some documentation in `render/surface.h`

## NEWS for 19.12.2024

* Removed the cplusplus ifdef in `core/vector.h`
    * The guard preveting `core/vector` from being used in c++ code (`#ifdef __cplusplus`) was causing github linguist to interpret the file as a c++ header lol

* Modernized the `platform/librtld` API
    * Added `p_librtld_load_lib_explicit`, which just loads the library without retrieving the symbols
        and gives more control over the file name, making you supply the file prefix and suffix,
        as well as the version string
    * Replaced `p_librtld_get_sym_handle` with `p_librtld_load_sym`,
        which, if the given sym is not found already loaded in the lib's symlist,
        will look for it in the shared object itself.
    * Renamed `p_sym_t` to `struct sym` in both platforms' code
    * Rerranged and refactored the code in the `librtld` sources
    * Added documentation in `platform/librtld.h`
    * Adapted the whole codebase to the changes
