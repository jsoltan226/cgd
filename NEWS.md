## NEWS for 23.04.2024

* Fixed some bugs, optimized `r_putpixel_fast*`
    * Removed the useless `for` loop in `spinlock_acquire`
    * Added a `u_macro_type_check` macro in `core/util.h`
    * Removed the duplicate `NULL` pointer check in `menu_oneven_api_switch_menu` (`gui/menu.c`)
    * Removed duplicate line in `platform/linux/keyboard-tty.c:117`
    * Fixed `s_assert` checking the wrong pointer in `p_librtld_load_lib_explicit` in `platform/*/librtld.c`
    * Fixed incorrect structuring of code in `mouse_evdev_update` in `platform/linux/mouse-evdev.c`
    * Forced the inlining of `r_putpixel_fast*` by defining them as macros instead of `static inline void` functions
    * Since the type checking on the new `r_putpixel_fast*` is now very strict, found and fixed a couple of implicit int conversions

## NEWS for 28.04.2024

* Fixed `platform/linux/opengl` loading, cleaned up the `Makefile` a bit
    * Changes to `Makefile`:
        * Added `*tests-release` targets to enable compiling the tests with different `CFLAGS` in release builds
        * Added a `tests-clean` target
        * Cleaned up the unnecessary `CFLAGS += -fsanitize=address`... in `compile-tests` and `build-tests`
        * Replaced all `-fsanitize=address` with references to the new global variable `ASAN_FLAGS`, to make it easier to disable asan
    * Changes to `platform/linux/opengl`:
        * Fixed `_voidp_eglGetProcAddress` and `eglGetProcAddress` not being in a `union`
        * Added missing call to `eglDestroyContext` in `terminate_egl`
        * Added missing call to `eglMakeCurrent(EGL_NO_CONTEXT)` in `terminate_egl`
        * Made `p_opengl_destroy_context` set the acceleration mode to `P_WINDOW_ACCELERATION_UNSET_`
            instead of `P_WINDOW_ACCELERATION_NONE` (software rendering)
        * Fixed a typo
    * Miscellaneous changes:
        * Added handling for `P_WINDOW_ACCELERATION_UNSET_` in `*window*_set_acceleration` in `platform/linux/window*`
        * `core/log.h`: Made `s_log_trace` and `s_log_debug` be replaced by `((void)0)` when disabled,
            instead of nothing (which would sometimes upset the compiler with pedantic settings)

* Made mouse deregistration in `platform/linux/window-x11` thread-safe
    * Just what the title says
