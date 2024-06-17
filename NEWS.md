## NEWS for Sat 15.06.2024

* "Refactored everything in `core/`, added tests for `hashmap` and `linked-list`" <jsoltan226@github.com>
    * Changed the coding style to the new one in all files (that were still written the old way) in `core/`
    * Renamed `hashtable` to `hashmap`
    * Added `hashmaptest.c` and `linked-list-test.c`
    * Added comments in header files in `core/`
    * Added proper implementations of `min()` and `max()` in `core/math.c`, because the C standard doesn't provide them lol
    * Added logging wherever applicable in `core/`
    * The `Makefile` will now also link the binary against libMath (`-lm`, `libm.so`)
    * Removed unused and deprecated functions and types, like `u_error()`, `u_Color`
    * Moved `u_color_arg_expand` to `core/util.h`, and removed `core/function-arg-macros.h`
    * Fixed some unintialized variable warnings in `io-PNG.c` and `asset-load-test.c` when compiling with clang

* "Refactored `gui/on-event.h`, `gui/sprite`, `gui/buttons`, `gui/event-listener` and `gui/parallax-bg`" <jsoltan226@github.com>
    * Just what the title says


## NEWS for Sun 16.06.2024

* "Refactored `gui/fonts`, started working on `gui/menu`" <jsoltan226@github.com>
    * Just what the title says

* "Added a C implementation of the C++ `std::vector`" <jsoltan226@github.com>
    * Added a pretty much full implementation of the `std::vector` in `core/datastruct/vector`. It. Was. A. Pain.
    * Added `tests/vector-test.c`
    * Fixed some uninitialized variable errors with clang in `gui/fonts.c` that were leftover from the previous commit

## NEWS for Mon 17.06.2024
* "Fixed `core/datastruct/vector`, added `s_log_fatal`, fixed some minor bugs" <jsoltan226@github.com>
    * `core/datastruct/vector` no functions (kinda)
    * Made `tests/vector-test.c` to be more of a fuzz test
    * The internal functions in `core/datastruct/vector.h` are now secured from the user with preprocessor guards
    * Fixed `CFLAGS` in `Makefile` for targets `release`, `build-tests` and `compile-tests`
    * Corrected some typos
    * Added the proper compiler flags in .clangd

* "Finished refactoring" <jsoltan226@github.com>
    * Added checks for NULL in arguments of pointer types pretty much everywhere
    * Started using `s_log_fatal` where appropriate
    * Made all the config structs fixe-sized
    * Refactored `gui/menu`, `gui/menu-mgr` and `gui/parallax-bg`, made heave use of `core/datastruct/vector`
    * Removed the preprocessor guards in `core/datastruct/vector` as they didn't work at all (lol), instead renamed internal functions to have a suffix of `__`, e.g. `vector_realloc__`
    * Fixed the way switching menus was happening in `gui/menu-mgr`
