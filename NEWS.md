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
