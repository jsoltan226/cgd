## NEWS for 21.07.2025

* Fixed some more MAAAJJOORRRR issues in `platform/windows`
    * Finally fixed `platform/windows/entry-point.c`!!!!!:
        * Fixed heap buffer overflow - only one index being allocated for the `argv` array even if there were more arguments to store
        * Fixed the incorrect use of `u_nzfree` (`u_nzfree(argv)` -> `u_nzfree(&argv`)
        * Fixed THE VERY FACT THAT `u_nzfree` WAS USED ON A HEAP ARRAY!
            * Two fucking memory corruption bugs in one fucking line lmao
    * Switched from using `SetDIBitsToDevice` with `malloc`ed buffers to `BitBlt` with `BITMAP`s,
        for better performance basically
    * Fixed the incorrect uses of `HDC`s (GDI device contexts)
    * Implemented proper checks to avoid accidental accessing of the front buffer by the renderer
        (buffer locking)
    * Made `RENDER_PRESENT_SOFTWARE` an actual non-blocking request lol
    * Further improved upon the fucked up architecture of the window thread request stuff:
        * Encapsulated sending the requests in convenient wrapper functions, greatly cleaning up the code in `platform/windows/window.c`
        * Removed the request-specific code from `platform/windows/window-thread.c` -
            now the individual modules implement the whole request-arg-unpacking-and-deciding-which-function-to-call stuff.
            This greatly cleaned up the code in `window_thread_fn`.
        * Cleaned up the mess that was passing arguments to/from the thread during init
    * Renamed `do_init_window` and `do_cleanup_window` to `create_window` and `destroy_window`, respectively
    * Fixed the incorrect use of `PostMessage` in `p_window_close`
    * Added documentation for `struct window_thread_request`, `struct window_render_software_ctx` and `struct p_window`
    * Fixed some regular writes to `_Atomic` variables in `platform/linux/window-x11`
    * `tests/bin/window-test.exe` was run 500 times without encountering any errors! FINALLY!

* Minor improvements & bug fixes
    * Fixed `ASAN_FLAGS` not being appended to `SO_LDFLAGS` in the `build-tests` and `build-tests-release` targets in the `Makefile`
    * Fixed an error when compiling with clang caused by the use of an `_Atomic LARGE_INTEGER`
        which clang doesn't allow since `LARGE_INTEGER` is not a primitive type
    * Added more documentation in `platform/windows/window`, `platform/windows/window-thread` and `platform/windows/window-present-sw`
    * Cleaned up the confusing definintions, guards and undefinitions from `platform/window.h`

* Implemented `platform/opengl` on Windows
    * Implemented an OpenGL loader for windows
        * Prepares the window
        * Loads the Windows GL (WGL) library (`opengl32.dll`)
        * Creates and binds WGL context
        * Tries to look up the given OpenGL functions inside `opengl32.dll`
        * If that fails, uses `wglGetProcAddress` instead
        * simpler than I thought it would be
    * `platform/windows/window-present-sw.c`: Fixed the bitmap `biHeight` parameter being initialized incorrectly
        causing all frames to be renderer upside-down
