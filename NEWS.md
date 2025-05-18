## NEWS for 18.05.2025

* Implemented presentation buffer locking in `platform/linux/window-x11`
    * Added a new field in `struct window_x11_render_software_ctx` - `presentation_pending`
        which is used to communicate between the caller of `p_window_swap_buffers` and the listener thread
        about whether a frame is awaiting a presentation. `render_software_present` will set the variable to `true`,
        while the listener thread will reset it back to `false` once it receives a `PRESENT_COMPLETE_NOTIFY` event.
        `render_software_present` will also drop the frame if another one is yet to be presented
        (the field is set when `render_software_present` is called).
        * Note that the above only applies to `X11_SWFB_PRESENT_PIXMAP`, since only it requires buffer locking.
            In all other (`X11_SWFB_MALLOCED_IMAGE` and `X11_SWFB_SHMSEG`) the field is reset at the end of `render_software_present`.
    * The `serial` field will now be incremented BEFORE the present request.
        This makes it easy to check for missed presentations in the listener thread.
    * `handle_present_event` in `platform/linux/window-x11-events` will now properly check
        for missed page flips and send the appropriate events (using the aforementioned `serial` field)
    * Fixed a bug where if `conn` wasn't `NULL` but the connection failed, `window_X11_close`
        would attempt to destroy resources that weren't yet initialized
    * Added some testing regarding page flip events/timing in `tests/window-test`
