## NEWS for 22.01.2025

* Changed the way software rendering is done in `platform/linux/window`, made `window-fbdev` support vsync
    * `p_window_swap_buffers` replaced `p_window_render` in software-rendered windows.
        `p_window_swap_buffers` will schedule the current back buffer for display at vsync,
        and return a new back buffer for rendering.
    * What this means is that double-buffering is now built into `platform/window` and the renderer doesn't have to manage that.
    * `window-fbdev` now supports vsync and sends `P_EVENT_PAGE_FLIP` events when after successfully presenting a front buffer
    * Note that work has only been done for `window-fbdev`; support for `window-x11` and `window-dri` is yet to come
