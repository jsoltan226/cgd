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
