## NEWS for 25.11.2024
* Minor changes and bug fixes
    * Adapted the whole thread subsystem to `p_mt_thread_wait` returning `void`
    * Removed the `flags` field from `p_mt_thread_create`
    * Deleted the now useless declarations of `window_X11_attach_fb` and `window_X11_detach_fb`
    * Finally fully fixed the async-signal safety issues in `platform/linux/event`
