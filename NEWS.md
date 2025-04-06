## NEWS for 02.04.2025

* Did some work in `platform/window`, merged changes and bug fixes from ps4-controller-input-faker
    * Merged the new `evdev` interface (from [ps4-controller-input-faker](https://github.com/jsoltan226/ps4-controller-input-faker)):
        * `evdev_find_and_load_devices` (and `evdev_load`) will now take in an `evdev_mask` as a parameter.
            This allows it to search for more than one type of device.
        * Added `evdev_destroy` and `evdev_list_destroy`
        * Refactored existing code and fixed a couple of small bugs
    * Started work on implementing the new `platform/window` interface in `platform/linux/window-x11`
        * Basic support for software rendering is mostly complete
    * (in `core/util.h`) `filepath_t` was renamed to `u_filepath_t` and it's type was changed to `char [256]` (previously it was `const char [256]`).
        Also `u_FILEPATH_SIZE` now describes the buffer SIZE while `u_FILEPATH_MAX` is the max STRING LENGTH (w/o the NULL terminator)

## NEWS for 06.04.2025

* Fixed design flaws in `core/vector` and `core/log` (and also a couple of small bugs)
    * Changes in `core/log`:
        * `s_log_fatal` (the function) was renamed to `s_abort`
        * `s_log_fatal` is now a macro that wraps around the new `s_abort`, ending the existence of these: `s_log_fatal(MODULE_NAME, __func__, ...)`
        * *Fixed a flaw that often lead to use-after-free conditions:* 
            **`s_set_log_out_filep` and `s_set_log_err_filep` now take a pointer to a file handle**
            so that when another function closes or resets the log file,
            the handle in the original caller to `s_set_log_*_filep` gets properly invalidated
        * Adapted the whole codebase to these changes
    * Changes in `core/vector`:
        * The vectors now have a minimum capacity of 8, meaning that they will never shrink to 0
        * Since I now know how the pointer arithmetic slop works,
            the definition of `struct vector_metadata__` was moved back to `core/vector.c` Making the Vector Opaque Again
            (`vector_size` still works by offsetting `VECTOR_METADATA_SIZE__` (defined in `core/vector.h`) behind the vector
        * *Fixed a flaw that often lead to use-after-free conditions:*
            **All functions that write to the vector in any form now take a vector pointer (`void **`) as an argument**
            in case `realloc` moves the pointer resulting in a use-after-free
        * Moved all code (except the direct value `__VA_ARGS__` assignments in `vector_push_back` and `vector_insert`) to the source `core/vector.c`
        * Since `vector_realloc`, `vector_increase_size` and `vector_memmove` are no longer needed in the header `core/vector.h`,
            they were moved to the source `core/vector.c` and declared `static`
        * Added checks that ensure that the the pointer to the vector (`void **`) is passed to the relevant functions,
            and it actually works!! (well, except for vectors of pointers, but that's still something)
        * Added bounds checking to most of the vector functions
        * Added more tests in `tests/vector-test`
        * Fixed the use of the default `goto_error` in `tests/vector-test` causing errors to not be reported
        * Adapted the whole codebase to these changes
    * Fixed the unused variable error in release builds in `platform/linux/opengl` (`load_opengl_functions`)
    * Removed some redundant `stdio.h` includes
    * Removed the `STRIP_OBJS`, `DEBUGSTRIP` and everything related to it from the `Makefile`
    * Fixed missing `MODULE_NAME` in `core/pressable-obj.c`
    * Updated the `.gitignore`
