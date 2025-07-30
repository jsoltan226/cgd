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

* Made `platform/*/entry-point.c` be in charge of setting up logging
    * Both `platform/windows/entry-point.c` and `platform/linux/entry-point.c` now handle logging setup.
    * It's configurable at runtime with the following environment variables:
        - `CGD_LOG_FILE` - The path to the log file. Default: `"log.txt"`
        - `CGD_NO_LOG_FILE` - Whether the entry point should skip setting up the log file altogether.
            This overrides the settings of `CGD_LOG_FILE` and `CGD_LOG_APPEND`. Default: `false`
        - `CGD_LOG_LEVEL` - The (initial) log level. Default: `S_LOG_INFO`
        - `CGD_LOG_APPEND` - Whether the log file should be opened in append mode. Default: `true`
        - `CGD_VERBOSE_LOG_SETUP` - Whether any messages should be `s_log`ged before calling main. Default: `true`
    * Removed the log setup code from `game/init.c` and `game/main.c`
    * Adapted the `Makefile` to set different default values for the above variables when building `tests`
    * Fixed `core/log.c` not flushing `S_LOG_OUTPUT_FILE` streams in `destroy_old_output`
    * Fixed missing `LocalFree` in `platform/windows/entry-point.c`
    * Fixed the usage of `u_nzfree` on a heap array in `platform/windows/entry-point.c`. AGAIN!!!!!
    * Added a `NULL` terminator to the `argv` array in `platform/windows/entry-point.c`
    * Removed the incorrect `argc` setting in `platform/windows/entry-point.c`
        * I swear the code in this file is just coming straight out of my ass lmao
