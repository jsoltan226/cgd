## NEWS for 06.09.2025

* Changed the way `P_WINDOW_ACCELERATION_OPENGL` is initialized and fixed a couple of bugs
    * Made it so that the window's acceleration must be set to `P_WINDOW_ACCELERATION_OPENGL` by the user,
        before the call to `p_opengl_create_context` (i.e. `p_opengl_create_context` doesn't itself set the window's acceleration to OpenGL)
    * Refactored the `check_egl_support` function in `platform/linux/opengl`, to improve it's reliability & maintainability
    * Added documentation in `platform/common/util-window.h`
    * Fixed the acceleration value not being initialized to `P_WINDOW_ACCELERATION_UNSET_`
        before the call to `pc_window_initialize_acceleration_from_flags` in `platform/linux/window-dri`
    * Fixed missing failure condition check after a `malloc` in `platform/windows/librtld.c` and `platform/linux/librtld.c`
    * Added logging of the path of the shared object that's actually being loaded in `platform/*/librtld`

* Refactored the `s_log_output_cfg`'s `flag_*` field into actual bit mask flags defined in `enum s_log_config_flags`
    * Just what the title says
    * Obviously adapted the codebase to this change

* Refactored the generic tty management code to a new module `platform/linux/tty`
    * Moved all code that manages the tty (opens/closes the tty fd, sets raw mode, etc) to `platform/linux/tty`
    * Made `window-dri` also set the tty to raw mode
    * Renamed all `platform/linux/keyboard-*` functions from `XXX_keyboard_*` to `keyboard_XXX_*`,
        for consistency with the rest of the codebase
