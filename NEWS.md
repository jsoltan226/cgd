## NEWS for Mon 27.05.2024

* Commit "Implemented a more robust and scalable asset system, refactored some old code" <jsoltan226@github.com>
    * I don't have any time to write the NEWS, good luck reading the diff lol
    * Renamed NEWS to NEWS.md

## NEWS for Tue 28.05.2024

* Commit "Added support for tests in the Makefile" <jsoltan226@github.com>
    * The Makefile now supports tests. It's a total mess, please don't attempt reading it...
    * Rewrote the TODO list a bit
    * Got rid of the `find-max-strlen.c` file as the new Makefile doesn't need it
    * Removed `.gitignore` from `.gitignore`

## NEWS for Thu 30.05.2024

* Commit "Added log.c and log.h" <jsoltan226@github.com>
    * Finally, huh?

* Commit "Implemented logging in `main()`" <jsoltan226@github.com>
    * Implemented logging in `main()`. If `argv[0]` is `"debug"`, the log level will be set to `LOG_DEBUG`. Otherwise it will be `LOG_INFO`.
    * Fixed the handling of varargs in the `s_log_debug`, `s_log_info`, `s_log_warn` and `s_log_error` macros
    * Header files in `platform/` are no longer different in each platform (It's just the implementations int the source files that are supposed to differ; the API for them should be the same!)
    * Made the error handling in `main()` cleaner and less insane

* Commit "Implemented logging in the asset loader" <jsoltan226@gmail.com>
    * Implemented logging in all files in `asset-loader/`
    * Fixed some trailing '/' issues in `u_get_asset_dir`
    * Fixed a bug related to unintialized memory usage from `malloc()` in `asset_load()`

* Commit "Added `asset-load-test.c`"
    * Added a test that fuzzes `asset_load` with random PNG images, present in `assets/tests/random_pngs.7z`
    * `obj/log.c.o` will now be stripped regardless of build type, for convenience during debugging
    * Added test hooks in the Makefile (to perform actions that a test depends on, e.g. extract an archive with needed files)
