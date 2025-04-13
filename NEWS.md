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
