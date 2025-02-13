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
