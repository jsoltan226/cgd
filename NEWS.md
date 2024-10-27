## NEWS for Sat 26.10.2024

* Minor changes and bug fixes
    * Made `asset_load` keep track of the number of created asset structures and unload all plugins in `asset_destroy` when that number reaches 0.
    This way we can avoid the mysterious call to `asset_unload_all_plugins` at the end of tests and the main game.
    * Updated `asset-load-test` accordingly
    * Enabled link-time optimization in release builds
    * Fixed `u_nzfree` and `u_nfree` not actually setting the pointer to `NULL`
    * Fixed the libpng interlace handling error messages in `io-PNG`
    * Made `io-PNG` use custom libPNG error and warning handler functions

* Implemented a wrapper for platform multithreading
    * Implemented a wrapper for linux AND WINDOWS (!!!!!) threads and mutexes
    * Renamed `window_X11_bind_fb` to `window_X11_attach_fb` (and also obviously `window_X11_unbind_fb` to `window_X11_detach_fb`)
    * Added a separate `.clangd` file for windows development

## NEWS for Sun 27.10.2024
* Further enhanced the platform mutex wrapper implementation
    * Static mutexes can be initialized with `P_MT_MUTEX_INITIALIZER` to register them for automatic cleanup upon exit.
        Or in other words, manual management of global mutexes is no longer needed.
    * `p_mt_mutex_lock` will now create a new mutex if the given mutex pointer points to `NULL` (or `P_MT_MUTEX_INITIAZLIER`).
    * Made use of these mutexes to make places that use global variables thread-safe.
    * Made `vector_destroy` properly set the vector handle to `NULL`.

* Added a windows implementation for `platform/event`
    * Just copied whatever was in the linux implementation, leaving out the signal handlers lol
