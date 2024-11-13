## NEWS for Thu 07.11.2024

* Finished making a platform layer for windows
    * ADDED FUCKING WINDOWS SUPPORT!!!!!
    * IT ACTUALLY KINDA WORKS!!!!!!!!
    * Updated .gitignore to include temporary files such as `log.txt` or `test.c`

## NEWS for Sat 09.11.2024
* actual minor changes
    * In `cgd_main`, logs will now be written to the log file (instead of `stdout`/`stderr`) only on the windows platform
    * Fixed `platform/platform.h` making it actually useful
    * Linked `.clangd` to `.clangd-linux`
    * The function pointers in `r_surface_blit` are now actually pointers, not that it matters too much lol

## NEWS for Sun 10.11.2024
* Cleaned up `game/main.c`, moving most of the code to `game/init` and `game/main-loop`
    * What the commit title says
    * `const`-ed out everything that was using `p_mouse` and `p_keyboard` but didn't need to modify anything
    * `p_mouse_get_state` and `p_mouse_update` are now separate functions and actually do what they are supposed to
    * Removed the `flags` argument from `p_mouse_init` (it was not being used anywhere)
