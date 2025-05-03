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
