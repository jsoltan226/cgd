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
