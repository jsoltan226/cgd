## NEWS for 22.12.2024
* Minor changes and bug fixes, mainly in `platform/linux/*-x11` and `render/surface`
    * Finally fixed the use of `%b` in a format string in `platform/linux/window-x11`
    * Removed the unprocessed event queue in `platform/linux/keyboard-x11`,
        because it provided no real benefits while causing a multidude of
        threading-related data race issues
    * `render/surface`:
        * Fixed a bug in `*_unconverted_blit` where `src_pixel` was being calculated inappropriately
        * Added `*_alpha` and `*_noalhpa` versions of all blit functions
        * Added support for the `RGBX32` and `BGRX32` color formats
        * Added proper checks and error messages when trying to use the `RGB24` and `BGR24` formats
    * Renamed `platform/linux/window-fbdev` to `platform/linux/window-dri`
        (for now that's just a name change, but I'm working on replacing
        the fbdev interface with the more modern DRI/KMS)
    * Started work on Vulkan and OpenGL renderers (out of tree)

## NEWS for 24.12.2024
* Added `window_dri_open` and `window_dri_close` in `platform/linux/window`
    * Added the open and close functions for `window-dri`
    * `window_dri_open` will select the best graphics card and pick it's best mode
    * `window-fbdev` was kept as a fallback option for when `window-dri` fails
    * Temporarily added `-ldrm` and `-lgbm` libs in the `Makefile` (just for the linux target)
