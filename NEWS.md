## NEWS for 12.06.2025

* Fixed some possible race conditions in `platform/linux/window-x11`
    * Added a `spinlock_init` function in `core/spinlock`
    * Made `spinlock_release` use explicit memory ordering
    * `platform/linux/window-x11-present-sw`: in `struct x11_render_shared_*_data`:
        Added separation between fields that are initialized once and stay the same
        throughout the lifespan of the window and the ones that constantly change -
        those that are constant are now in a nested anonymous struct `const_data`.
    * Each aforementioned `const_data` struct has a corresponding `const_data_lock`,
        which is used during initialization/destruction (when writing to the `const_data`)
    * In `platform/linux/window-x11-present-sw`, swapped out redundant uses of
        `_Atomic bool` for the more efficient `atomic_flag`
    * Added a `dropped_frames` counter in `struct window_x11_render_software_ctx`
        which gets quietly incremented when appropriate and then logged during window destruction.
        This is a much better solution than spamming the user with "Dropped frame!" warnings
    * Moved the setting of the `present_thread_ready` outside the loop, utilizing `goto` to enter it instead.
        While this does make the code a little less readable, it avoids the redundant setting of the flag
        on each loop iteration. For more details, see `platform/linux/window-x11-present-sw.c:920`
    * Fixed race condition: in `X11_render_destroy_software`, not using `atomic_load`
        to read the values of `sw_rctx->*_shared_data.initialized_` (which are `_Atomic bool`s)
    * Fixed race condition: in `X11_render_present_software`, the swapping of the
        `curr_front_buf` and `curr_back_buf` pointers is not an atomic operation, and so a data race is possible
        where a thread reads/writes the value of either one of the pointers during the swap.
        This was solved by adding a `swap_lock` spinlock.
    * Fixed race condition: `win->input.registered_keyboard` and `win->input.registered_mouse` weren't `_Atomic`,
        enabling a race condition in `window_X11_(de)register_(mouse|keyboard)`.
    * Fixed a mutex not being unlocked before getting destroyed in `window_X11_register_(mouse|keyboard)`
