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
