## NEWS for Wed 13.11.2024
* Added a queue for unprocessed events in `platform/linux/keyboard-x11` and fixed a couple of thread-safety related bugs
    * Finally fixed the bug in one of the error messages in `surface-test`
    * The linux `p_mt_cond_signal` implementation will now signal all threads waiting on a given cond var
    * Made X11 keyboard registration/deregistration thread-safe
    * Added queues for unprocessed key and button events in `platform/linux/window-x11`
    * Fixed a race condition in `platform/linux/event` caused by the `SIGTERM` signal handler

## NEWS for Mon 25.11.2024
* Added proper double-buffering in the renderer (kinda)
    * Abandoned the idea of 'binding a framebuffer to a window'
    * Got rid of `p_window_bind_fb` and `p_window_unbind_fb`
    * `p_window_render` will now just render a framebuffer given to it, instead of the window's current 'bound framebuffer'
    * Implemented buffer swapping in `render/rctx`, but it's basically useless right now (hence the 'kinda' in the commit title)
    * Made the return type of `p_mt_thread_wait` be `void` due to incompatibilities between platforms
