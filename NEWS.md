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
