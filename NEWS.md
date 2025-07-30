## NEWS for 21.07.2025

* Cleaned up the mess in `platform/windows/window*` a bit
    * The window thread is now signalled to terminate by a special (shared) variable
        instead of the event directly
    * Coupled the window thread's global data into a `struct window_thread_global_data`
    * In `struct window_thread_request`: renamed `status_mutex` to `request_mutex`
    * Rearranged/renamed the arguments to many functions to make them more readable & consistent
    * The `window_procedure` now has access to the thread's `global_data` through `(Set|Get)WindowLongPtr`,
        which allows it to e.g. set the aforementioned `running` variable
    * `p_window_close` will now `SendMessage` the quit event instead of `PostMessage`ing it
