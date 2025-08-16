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

* Fixed a bunch of `tests/`
    * Fixed `log-test` failing if the `S_LOG_LEVEL` environment variable was set to higher than `S_LOG_VERBOSE`
    * Fixed `test_log_setup` (from `tests/log-util.h`) overriding the log level set by the platform entry point
    * Adapted `tests/render-test` to the changes in `platform/window`
    * Fixed `tests/window-test` using micro-second sleep (`p_time_usleep`)
        where it should've been using milli-second sleep (`p_time_msleep`)

## NEWS for 15.08.2025
* Fixed broken functionality in `platform/linux/window-dri` and improved the consistency of the `p_window` API
    * Fixed incorrect handling of async page flips in `platform/linux/window-dri` -
        now the code actually checks that the driver supports them
    * Fixed `render_init_software` in `platform/linux/window-dri` using only 1 dumb buffer (it should've been 2)
    * Made `render_prepare_frame_*` in `platform/linux/window-dri` only take in what they need as arguments
    * Made `window_dri_swap_buffers` return errors correctly
    * Fixed not checking the return values of some I/O functions and X requests
    * Made the behavior of `p_window_*` more consistent:
        * `p_window_set_acceleration` (and consequently `p_window_open`) will now in all cases initialize the window's pixel buffers to 0
            - In `P_WINDOW_ACCELERATION_OPENGL` this is done by rendering 1 empty frame before `p_opengl_create_context` returns.
                This also makes it so that in the linux `window-dri` the modeset happens in `p_opengl_create_context`
                and not in the user's first call to `p_window_swap_buffers`.
            - In `P_WINDOW_ACCELERATION_NONE` this is simply done by using `calloc` instead of `malloc`
                (or simply `memset`ing the buffer to 0 if some other method of allocation is used).
        * `p_window_swap_buffers` will now always return `P_WINDOW_SWAP_BUFFERS_FAIL` on error (finally)
        * `p_window_swap_buffers` will only perform the actual buffer swap on success
    * Refactored `tests/bin/render-test`
    * Fixed no log file by default on windows

## NEWS for 16.08.2025
* Refactored the mouse & keyboard code in `platform/linux/window-x11`
    * Instead of duplicating the identical `registered_mouse_*` and `registered_keyboard_*` everywhere,
        registered stuff is now handled as a generic `x11_registered_input_obj`.
        This basically means that if the need arises for a new class of X11 input devices,
        they can be easily added without all the clobber.
        This change also greatly improved the general readability & maintainability of `window-x11`.
    * Refactored some of the code impacted by the aforementioned change in `handle_event` in `platform/linux/window-x11-events`
    * Fixed the incorrect use of `pthread_cond`s in `window_X11_(de)register_*`
    * Fixed escape sequences being stripped even when the log output is `stdout/stderr` in `platform/linux/entry-point`
    * Fixed not checking the return values of some X11 requests and I/O functions specifically in `platform/linux/window-x11`
