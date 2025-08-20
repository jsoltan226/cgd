## NEWS for 20.08.2025

* Refactored `platform/*/*` code that was repeated across platforms into `platform/common/`
    * Moved a couple common functions from `platform/*/window*` to `platform/common/util-window`:
        - `check_flags` became `pc_window_check_flags`
        - `set_default_flags` became `pc_window_check_flags`
        - The various `initialize_acceleration`s became the generic `pc_window_initialize_acceleration_from_flags`
    * Added a common `platform/common/guard.h` to replace the `#ifndef P_INTERNAL_GUARD__`... found everywhere in `platform/`
    * Fixed a bunch of missing header guards in `platform/*/*`
    * Fixed using `atomic_store` and `atomic_load` instead of `atomic_exchange` in `platform/linux/event`
    * Fixed a typo in `asset-loader/plugin-registry.h`
