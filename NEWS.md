## NEWS for 22.01.2025

* Changed the way software rendering is done in `platform/linux/window`, made `window-fbdev` support vsync
    * `p_window_swap_buffers` replaced `p_window_render` in software-rendered windows.
        `p_window_swap_buffers` will schedule the current back buffer for display at vsync,
        and return a new back buffer for rendering.
    * What this means is that double-buffering is now built into `platform/window` and the renderer doesn't have to manage that.
    * `window-fbdev` now supports vsync and sends `P_EVENT_PAGE_FLIP` events when after successfully presenting a front buffer
    * Note that work has only been done for `window-fbdev`; support for `window-x11` and `window-dri` is yet to come

## NEWS for 13.02.2025
* Multiple fixes & enhancements in `platform/linux/window`
    * Fixed the fallback crtc selection in `find_crtc` (from `platform/linux/window-dri`)
    * Added checks for mutually exclusive flags in `p_window_open`
    * Fixed `platform/linux/window-fbdev` reporting the window size as the display size
    * Added more checks in `platform/linux/opengl`
    * Moved `enum window_acceleration` from `platform/linux/window` to the generic `platform/window`
    * Made `p_window_set_acceleration` and it's implementations return non-zero on failure instead of aborting
    * Made the listener thread in `platform/linux/window-fbdev` block until another frame is ready for presenting
    * Fixed the incorrect order of operations in rendering in `platform/linux/window-fbdev`

## NEWS for 26.02.2025
* Reworked many things in `platform/window`, `platform/linux/window-dri` and `platform/linux/window-fbdev`
    * The `title` argument of `p_window_open` is not `const char *` instead of `const unsigned char *`
    * Fixed a segfault in `platform/linux/keyboard-evdev.c` caused by the incorrect order of incrementation/comparison
        (`do { ... } while (i++ < P_KEYBOARD_N_KEYS)` -> `do { ... } while (++i < P_KEYBOARD_N_KEYS`)
    * Reworked the `p_window_get_meta` function:
        * Renamed the function to `p_window_get_info` and the struct to `struct p_window_info`
        * Added fields for display rect and vsync support, made the client area be a `rect_t`
        * In `platform/linux`, this info is stored in a separate struct in the generic `struct p_window`,
            and a pointer to it is passed to the window type in use
        * Renamed `ev_offset` to `mouse_ev_offset` in `struct p_window` in `platform/linux`
    * Changes in `platform/linux/window-dri`:
        * Renamed `window_dri_render` to `window_dri_swap_buffers`
        * Renamed the acceleration-specific `window_dri_render_...` to `render_prepare_frame_...`
        * Renamed `finish_frame` to `render_finish_frame`
        * Renamed `perform_drm_page_flip` to `render_present_frame`
        * Renamed the `window_dri_listener_signal_handler` to the more appropriate `do_nothing`
        * Added separate functions for loading libgbm and libdrm to clean up `window_dri_open`
        * Moved the calls to `render_present_frame` into a single one at the end of `window_dri_swap_buffers`
        * Added the `O_CLOEXEC` flag to all calls to `open()`
        * Added a function to properly initialize GPU acceleration according to the flags
        * Software rendering now uses a dumb buffer instead of a GBM surface
        * Due to the above change, libgbm is only needed for the EGL/OpenGL acceleration mode,
            and so any code related to it (including loading the library) got moved into the
            EGL/OpenGL acceleration specific functions
        * Set the CRTC when drawing the first frame with EGL/OpenGL
        * `drmModePageFlip`'s flags are now adjusted based on the specified VSync present mode
        * `render_destroy_egl` now actually cleans everything up properly
    * Changes in `platform/linux/window-fbdev`:
        * `window_fbdev_open`: Implemented correct handling of all flags
        * Changed the pixclock warning to a debug message
        * Everything line is now at max 80 characters long lol
    * Implemented proper default flag settings in `platform/linux`'s `p_window_open`
    * In both `window_dri_prepare_frame_software` and `window_fbdev_swap_buffers`
        the window framebuffer blitting now uses `u8 *` instead of `void *`/`pixel_t *`,
        because pointer arithmetic is confusing
    * Added calls to `p_window_swap_buffers` in the `opengl_test` in `tests/render-test`
    * Fixed many typos and overly long lines

