## NEWS for 27.02.2025

* Made the whole codebase (except for `platform/windows`) strictly compliant with C11
    * The most boring change you could think of, just read the diff if you want to lol
    * Although it's worth noting that I actually caught a couple of bugs with this!
    * Added some documentation in `platform/keyboard.h`
    * Removed (the pretty much empty) `platform/linux/wm` as it wasn't being used anyway

* Made `platform/windows` C11 compliant
    * What the title says
    * Also began adapting the windows implementation `p_window` to the newer API
        (e.g. properly implemented `p_window_get_info`)
    * Obviously the executable can't link due to the missing functions,
        but at least everything compiles with no warnings
