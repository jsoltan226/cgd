## NEWS for 03.05.2025
* Refactored the software rendering backend in `platform/linux/window-x11`
    * Made the present buffer a union of one of three types: `X11_SWFB_MALLOCED_IMAGE`, `X11_SWFB_SHMSEG`, or `X11_SWFB_PRESENT_PIXMAP`.
        `X11_SWFB_PRESENT_PIXMAP` is planned for the future implementation of a vsync-compatible present mode, and is for now empty.
    * Split the buffer-type-specific logic `render_*_software` into `software_*_init`, `software_*_present` and `software_*_destroy`
    * Ensured that the integers passed to the X11 API are of compatible size
        (most notably - `p_window_open` accepts a `rect_t` (`{ i32 x, y; u32 w; h}`) while X11 can only handle `{ i16 x, y; u16 w, h; }`).
    * Renamed the `libxcb_error` macro in `window_X11_open` to `libxcb_error()` to highlight that it's more of a function than a constant
    * `window_X11_close` now calls `p_window_set_acceleration` to destroy any acceleration-specific resources, instead of doing it manually
    * Added an error condition when attempting to present a frame with vsync
    * Removed the pixmap from `X11_SWFB_SHMSEG` as it didn't provide any benefits while introducing potential overhead
    * Added checks to all calls to `xcb_flush`
    * `render/rctx.c`: Added checks for vsync support in `r_ctx_init` and `r_flush`

## NEWS for 08.05.2025

* Major changes in and around `platform/linux/window-x11`
    * `platform/linux/libxcb-rtld`:
        * Added the `libxcb-present` library and functions
        * Made `libxcb-xinput` load as an extension, the same way as `libxcb-shm` and `libxcb-present`,
            because it's not part of the core X11 protocol nor an xcb utility
        * Added support for loading variable symbols by splitting the `X_` symbol definitions into `X_FN_` for functions
            and `X_V_` for variables
        * Moved the library (un)loading itself to separate functions: `do_load_libraries` and `do_unload_libraries`
        * Refactored the repeated macros for loading symbols to make the shorter and cleaner, hopefully
        * Renamed `ERROR_MAGIC` to `LIBXCB_RTLD_ERROR_MAGIC`
        * Changed the `failed_` field in `struct libxcb` to `loaded_`
        * Added definitions for X11 extension names in `libxcb-rtld.h`
    * `platform/linux/window-x11-events`:
        * Started using `xcb_ge_generic_error_t` instead of the deprecated `xcb_ge_error_t`
        * Split the `handle_ge_event` function into separate ones for XInput2 and Present events
        * Added guards for a deregistered mouse in `handle_xi2_event`, which were already present for the keyboard
            but I somehow forgot to also implement them for the mouse
    * `platform/linux/window-x11`:
        * Moved ANYTHING related to software rendering/presentation to a new module - `platform/linux/window-x11-present-sw`
        * Properly implemented the `X11_SWFB_PRESENT_PIXMAP` presentation mode
        * Added functions to properly check for extension availability, both server side and client side
        * Implemented vsync support
    * Fixed a race condition in `platform/linux/window-dri`, `platform/linux/window-x11` and `platform/windows/window`,
        where a frame presentation could occur during the call to `p_window_set_acceleration`
        after the resources of the current acceleration mode have been destroyed, but the `gpu_accleration` field
        was still set to the old acceleration instead of `P_WINDOW_ACCELERATION_UNSET_`
    * Added missing `P_WINDOW_NO_ACCELERATION` flags in some tests
