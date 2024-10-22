## NEWS for Sun 20.10.2024

* Fixed `render/line`, `window-dummy` and `asset-load-test`
    * Made `r_draw_line` actually... draw lines!
    * Updated `asset-load-test` to use `p_time` instead of the unix-specific `sys/time.h`
    * Added framebuffer bind/unbind functions to `window-dummy`, making it actually useful when only dealing with a renderer and no input handling (which is the case in `asset-load-test`)
    * Moved `assets/tests/random_pngs.7z` to `assets/tests/asset_load_test` to avoid cluttering the main `assets/tests` directory
    * Finished writing `surface-test`
    * Updated `TODO`, probably for the first time in months

## NEWS for Tue 22.10.2024
* Fixed the scaling issues in `r_surface_blit`
    * Renamed keyboard 'mode' to 'type' in `platform/linux/keyboard`
    * Fixed OOB reads and bad scaling due to improper handling of src_rects in `r_surface_blit`
