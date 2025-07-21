## NEWS for 08.07.2025

* Added a clean API around X11 extensions
    * Added an API around X11 extensions (`platform/linux/window-x11-extensions`):
        * Initialized in early stages of `window_X11_open`
        * Caches the data as it doesn't change during the lifetime of a connection
        * Is thread-safe
    * Implemented the use of said API in various places (including `event_listener_fn`)
    * Removed the `const_data` structs from `window-x11-present-sw`, because since it doesn't change,
        it's better to simply pass the arguments to the respective threads during their creation
        and having them store the data locally instead of accessing a "global" variable
    * In `platform/linux/window-x11-present-sw`: grouped the common window parameter data into 
        `struct x11_render_software_generic_window_info`, greatly simplifying passing this data to different
        software-presenting related functions (which previously had like 18 parameters each)
    * Replaced regular `pthread_join`s with the more robust `pthread_timedjoin`s
    * Added proper assertions/checks with `pthread` API functions
    * Added a function `spinlock_try_acquire` (`core/spinlock`)

* Completely remade  the internal architecture of `platform/windows/window`, making it finally kinda work
    * Separated code related to software rendering/presentation out into the new `platform/windows/window-present-sw`
    * Did the same for anything related to the listener thread (moved the code to `platform/windows/window-thread`)
    * Reworked the entire window rendering/presenting internal API architecture:
        * Since most operations on the window handles should be done by the thread that owns the window
            (the window listener thread), things had to be changed so that only the listener thread calls
            the actual functions (like `render_init_software`, `render_present_software`) even though the user
            requested the operations from a whole another thread.
        * And so now the architecture looks something like this:
            * The user calls the `p_window_*` API, which btw remains unchanged
            * `p_window_*` calls `window_thread_request_operation`, passing the ID of the requested operation
                (defined in `platform/windows/window-thread.h`) as well as a `void *arg` as the argument
                for that specific operation.
            * A condition variable may be optionally supplied to be signalled when the request is complete.
            * `window_thread_request_operation` `PostMessage`s a custom `WM_APP + x` event to the listener thread
            * The listener thread handles the request by calling the appropriate function (e.g. `render_init_software`),
                unpacking any arguments passed with the `void *arg`
            * When the operation is complete, the listener thread sets the `status` member in the struct
                in accordance to the result of the operation, signals the condition variable (if provided),
                and then returns back to the main event loop.
            * The `p_window_*` function inspects the status and results of the operation and returns it
                to the user according to the good old `p_window` API
        * This is the old architecture, for comparison:
            * The user calls the `p_window_*` API
            * The `p_window_*` function directly calls the internal function (e.g. `render_destroy_software`)
                that does the real work
            * The internal function returns a status that is interpreted by `p_window_*`
            * `p_window_*` returns the status according to the `p_window` API
        * A function `window_thread_request_operation_and_wait` is provided for convenience for when
            synchronous operation is desired so that code that creates a temporary condition variable,
            constructs the request etc doesn't get repeated all over the place
    * Moved the acceleration destruction to BEFORE the listener thread destruction in `p_window_close`
    * In `platform/windows/thread`: Replaced the `initialized` and `is_static` members of `struct p_mt_mutex`
        with `_Atomic` versions, because I couldn't stand it anymore lol
    * All of this (hopefully) got us safe from the sussy undefined behavior (deadlocks, crashes)
        that were randomly happening on windows
    * Fixed some typos ("WINWIN32_LEAN_AND_MEAN" -> "WIN32_LEAN_AND_MEAN")
    * Removed the useless `<vulkan/*.h>` includes in `tests/render-test.c`

* Ironed out the bugs in the new `platform/windows/window` implementation
    * Fixed the crash that would always occur on program exit:
        * In `window_thread_fn`, `GetMessage` now gets called inside the `while` condition check
            fixing a bug where the `REQ_OP_QUIT_` event would get through to `handle_thread_request`
            causing an abort (in `p_window_close`)
    * Fixed a race condition that would occur due to improper use of condition variables
        in `window_thread_request_operation_and_wait` and `window_thread_fn`:
        * The `status` variable is now protected my the mutex that's used with the cond.
            Previosly `status` was `_Atomic` and the mutex didn't protect anything.
            This caused a rare race condition where the request would be completed and `status` would get set
            right inbetween the `if (status == REQ_STATUS_PENDING)` and `p_mt_cond_wait(...)` lines,
            causing a deadlock.
    * Fixed the incorrect initialization of an atomic variable leading to a rare crash
        * The (now changed) `_Atomic` `status` variable would get initialized to 0
            with a non-atomic write, then it would get atomically set to the proper value.
            However, if the non-atomic write got "published" to the other thread AFTER the atomic write,
            the variable in the window thread would get set to 0 and cause an assetion failure.
            This is not a problem anymore as `status` is now handled completely differently, see the previous point
    * Added a `REQ_OP_MIN__` value for easier verification of the validitity of an `enum window_thread_request_status` value
    * The mutex used in `p_window_swap_buffers` will now be reused between calls
        instead of being created and destroyed on each request
    * Some other minor changes:
        * `r_flush` now treats a `NULL` return value from `p_window_swap_buffers` as just a dropped frame
            instead of a fatal error
            * As a result, removed the `dropped_frames` counter from `platform/linux/window-x11-present-sw`
                and made it return `NULL` on failure (instead of the old back buffer)
        * Removed the old definitions for `CGD_REQ_EV_*` as I was confusing them with the new ones in my code
        * Added proper `NULL` values for `p_mt_mutex`es and `p_mt_cond`s
        * Fixed some minor bugs in `core/log.c` because they were bothering me during debugging:
            * Fixed the error stream not being flushed by `do_abort_v`
            * Fixed the file output not being flushed for `S_LOG_OUTPUT_FILE` outputs in `destroy_old_output`
