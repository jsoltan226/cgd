## NEWS for 27.02.2025

* Made the whole codebase (except for `platform/windows`) strictly compliant with C11
    * The most boring change you could think of, just read the diff if you want to lol
    * Although it's worth noting that I actually caught a couple of bugs with this!
    * Added some documentation in `platform/keyboard.h`
    * Removed (the pretty much empty) `platform/linux/wm` as it wasn't being used anyway
