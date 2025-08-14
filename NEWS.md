## NEWS for 14.08.2025

* Fixed redrawing being broken when dragging window on Windows
    * Since the Win32 API normally enters a "modal loop" - where it basically takes over the window thread -
        during any move/resize operation, it disables and breaks any other functionality of the thread during that time.
        This, due to the design of the windows implementation of the `p_window` API, is unacceptable.
    * This means that I had to reimplement the dragging functionality manually, in a way that doesn't block the thread.
        * (Resizing is not ~yet~ supported (thankfully :/))
    * And so that's what I did. It works! Yay! (for now...)
    * The Win32 API is designed by a monkey on fent and you won't convince me otherwise

* Made the return value (errors) of `p_window_swap_buffers` not retarded
    * Since a `NULL` return value might actually sometimes indicate success
        (e.g. when `P_WINDOW_ACCELERATION_OPENGL` is used),
        added a special value - `P_WINDOW_SWAP_BUFFERS_FAIL` to indicate failure of the request.
    * Implemented this change to the API in `platform/windows`
    * Also fixed the documentation to make it clear that `p_window_swap_buffers` only "requests" the
        presentation and doesn't block until any results of the actual page flip are available.
    * Fixed a bunch of typos in `platform/window.h` & generally improved the documentation of `p_window_swap_buffers`
    * Fixed the `timestamp_delta` macro in `platform/ptime.h`

* Fixed the ridiculous logic of the linux implementation of `p_window_swap_buffers`
    * Replaced `if (param_invalid) return FAIL` with `u_check_params` where applicable
    * Implemented the `P_WINDOW_SWAP_BUFFERS_FAIL` return value in all window implementations except `window-dri`
    * Fixed the `vsync_supported` field in `struct p_window_info` not being changed properly in the linux `p_window_set_acceleration`
    * Actually properly handled the destruction of buffers in `platform/linux/window-x11-present-sw`
        (waiting for a signal that the buffers are no longer being used instead of just nuking them no matter what)
    * Fixed a bunch of typos all over `platform/linux/window*`
    * Implemented the change in the `p_window` API in `render/rctx.c`
