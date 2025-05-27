## NEWS for 27.05.2025

* Implemented proper non-blocking error-checking double-buffered presentations for all `platform/linux/window-x11` presentation modes
    * Made `X11_SWFB_SHMSEG` and `X11_SWFB_MALLOCED_IMAGE` presentations non-blocking and error-checking:
        * `X11_SWFB_SHMSEG` will now tell the `XShmPutImage` request to send a `ShmCompletion` event to the listener thread when the page flip completes successfully. The listener thread will then handle such events to mark the buffer as free and/or handle any errors by sending a `P_EVENT_PAGE_FLIP` event with a `page_flip_status` of FAILURE (`1`).
        * `X11_SWFB_MALLOCED_IMAGE` now spawns a completely separate thread, whose sole job is to write the framebuffer's data to the X11 socket with the X `PutImage` request, like so:
            * The malloced-present thread blocks on a condition variable while idle
            * `software_present_malloced` checks that the thread isn't already busy with another frame, and if it is, a failure status is returned
            * `software_present_malloced` signals the aforementioned condition variable and sets the framebuffer that is to be presented
            * `software_present_malloced` now returns with a success status
            * In the present thread, the framebuffer is split up into smaller chunks so that they fit within the X request size limit
            * The present thread now sends a series of `PutImage` requests, one chunk after the other
            * If everything goes alright, a `P_EVENT_PAGE_FLIP` with `page_flip_status` of SUCCESS (`0`), or FAILURE (`1`) if an error is encountered.
            * The present thread unlocks the buffer and goes idle, waiting on the present request condition variable
        * Due to the rewrite of the `X11_SWFB_MALLOCED_IMAGE` presentation code, no function from `libxcb-image.so` is needed anymore, and so it was entirely removed from `libxcb-rtld`
        * Made the event received by `window_X11_event_listener_fn` be freed explicitly in the main thread function instead of `handle_event`
        * Added support for `ShmCompletion` events in the listener thread
        * All multithreading code in `platform/linux/window-x11` now directly uses the `pthread` library instead of the `p_mt_*` wrapper
        * Removed some useless random lines of code in `handle_ge_event`
        * Fixed the `initialized_` fields in `sw_rctx->shared_data.*` not being initialized... lol
        * `X11_render_present_software` will only actually swap the buffers if the presentation request is successfull(y sent)
        * Refactored the `software_*_buffer_*` functions to take in as arguments only what they specifically need
