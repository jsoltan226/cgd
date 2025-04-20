## NEWS for 02.04.2025

* Rewrote the entire `core/log` module
    * Renamed the log levels `LOG_*` to `S_LOG_*`
    * Split `S_LOG_DEBUG` into `S_LOG_TRACE`, `S_LOG_DEBUG` and `S_LOG_VERBOSE`, where `S_LOG_VERBOSE` stays enabled in release builds
    * Each log level now has a separate configurable output, which means that e.g. `S_LOG_WARN` can go to a different file than `S_LOG_ERROR`
    * Added output stream types:
        * `S_LOG_OUTPUT_FILE` (user-managed `FILE *`),
        * `S_LOG_OUTPUT_FILEPATH` (`FILE *` automatically managed by `s_log_*`)
        * `S_LOG_OUTPUT_MEMBUF` (an in-memory <kinda> ring-buffer that's meant to be used for very early logs)
        * `S_LOG_OUTPUT_NONE` (level disabled)
    * `S_LOG_OUTPUT_FILEPATH` can be configured to open the file in append mode (instead of truncating the file and deleting it's previous content)
    * When switching the output from `S_LOG_OUTPUT_MEMBUF`, it's possible to set a flag to copy all the logs from the membuf to the new output
        (for example, after the early init when the proper log file is opened, it might be useful to also put the early logs there)
    * There's also an "extra" log level `S_LOG_DISABLE` to disable all logging entirely
    * Each log level also has a separate, configurable prefix string (previously that would be e.g. "WARNING: " for `LOG_WARN`)
    * The whole API is thread-safe (hopefully)
    * Added a helper function to clean up all log levels (`s_log_cleanup_all()`)
    * Added `core/spinlock`, a very simple wrapper around `atomic_flag`
    * Added some more helper macros in `core/ansi-esc-sequences.h`
    * Adapted the changes in `game/init` to make the code compile and (at last for now) everything works! Yay!

## NEWS for 19.04.2025

* Further improved `core/log` and fixed a couple of bugs
    * Changes to `core/log`:
        * Made the `S_LOG_OUTPUT_MEMBUF` log output be an actual ring buffer
        * Replaced `s_configure_log_prefix` with `s_configure_log_line`, allowing for a highly flexible configuration of log messages
        * The log line is now configured by using a format string, where `%m` represents the module name and `%s` - the actual message.
        * Due to this, logs are now written token-by-token (`linefmt_next_token`), where a token can be one of: "short string" (Anything inbetween the module name/message), the module name, the message itself.
        * Split `try_set_output_config` into `try_init_new_output`, `destroy_old_output` and `store_new_output` to improve readability (previously `try_set_output_config` was >100 lines long)
        * Set the maximum log message length (in `core/log.h`: `#define S_LOG_MAX_SIZE 4096`)
        * Fixed a logic error in `strip_esape_sequences`
    * Adapted everything in `tests/` to the new logging API
    * Added a small test for `core/log` (`tests/log-test.c`)
    * Changes to `Makefile`:
        * Added a `trace` target that enables `S_LOG_TRACE` during compilation
        * Made the `pthread` library be linked in only when targeting `linux`
        * Enable asan in the `all` (debug) and `trace` targets
    * Bug fixes:
        * Fixed a missing argument to `s_log_error` in `asset-loader/asset.c`
        * Fixed an invalid use of `vector_size` (`v_p` instead of `*v_p`) in `vector_insert_prepare__`
    * Replaced any `#warning`s with `#error`, since `#warning` is not a C11 standard feature

* Documented the logging API (`core/log.h`)
    * Exactly what the title says lol

* Minor bug fixes
    * Fixed yet another logic error in `membuf_write_string` in `core/log.c`
    * Fixed log messages being repeated when switching from a membuf to an output shared by multiple levels
    * Removed `-DCGD_ENABLE_TRACE` from both `.clangd`s
    * Fixed missing `void` in function prototypes in `asset-loader/plugin.h`
    * `core/vector.c`: Renamed the useless `is_const` member from `vector_meta_t` to `dummy_`
    * Added bounds checking to `vector_begin`
    * Fixed `vector_begin` and `vector_end` tests in `tests/vector-test.c`
    * Made all lines in `tests/vector-test.c` be <= 80 columns
    * Fixed `S_LOG_DISABLED` not doing anything, and added the missing documentation for it

* Made proper use of the new log levels (`S_LOG_TRACE` and `S_LOG_VERBOSE`) and fixed a build error
    * Replaced `s_log_debug` with `s_log_verbose` or `s_log_trace` where it was appropriate to do so
    * Fixed linker error when building `tests/libmain_test.so` in `release` with `CC=clang`, caused by missing `-flto` flags
    * Removed some redundant `stdio.h` and `stdlib.h` includes
    * Added missing `s_log_trace` definition after `#ifndef CGD_BUILDTYPE_RELEASE` in `core/log.h`

## NEWS for 20.04.2024
* Refactored the `S_LOG_OUTPUT_MEMBUF` implementation, creating `core/ringbuffer` in the process
    * `membuf_write_string` was moved to a new module - `core/ringbuffer`.
        This separates the generic ringbuffer funcionality from `core/log`, while also making
        everything cleaner and safer.
    * Fixed the (hopefully) last logic errors in `ringbuffer_write_string` (previously `membuf_write_string`)
    * Each level, instead of tracking it's own write index, has a pointer to a `struct ringbuffer`.
        This fixes the partially overwritten messages when using `S_LOG_OUTPUT_MEMBUF`,
        caused by each level sharing the same buffer, but having separate write indexes
        (so e.g. when `S_LOG_DEBUG` would write a message of 16 chars, it's write index would be
        updated properly, but now when `S_LOG_INFO` writes something to the same buffer,
        it does so according to it's own write index, which is 0, causing it to overwrite the previous
        `S_LOG_DEBUG`'s message.
    * The above change has also greatly simplified a lot of the code in `core/log.c`, making it cleaner and easier to maintain
    * Due to the changes to `struct s_log_output_cfg`, passing in `NULL` as the membuf base pointer
        to have `s_configure_log_output` manage the buffer for you is no longer possible,
        hence it got removed.
    * Improved the `log-test` a bit
