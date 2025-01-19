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

## NEWS for 19.01.2024
* Fixed a critical bug (OOB write) in `vector_push_back` in `core/vector.c`
    * `vector_increase_size__` (called in `vector_push_back`) calculated the new size
        with the formula
        `
        if (n_items >= capacity)
            capacity *= 2
        `
        If the capacity was zero, however, the `vector_push_back` function would not actually increase the size (0*2 = 0),
        but would still try to write to the end as though there was an extra slot.
        This is an OOB write, in a function used pretty much everywhere,
        with the parameters sometimes controlled by the user, so basically an easy ACE condition.
        The bug was triggered mainly by `p_event_send`/`p_event_poll`,
        (The queue would often be emptied and the refilled again)
        which is also where I spotted it.

