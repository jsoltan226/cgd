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
