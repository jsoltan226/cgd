## NEWS for Wed 13.11.2024
* Added a queue for unprocessed events in `platform/linux/keyboard-x11` and fixed a couple of thread-safety related bugs
    * Finally fixed the bug in one of the error messages in `surface-test`
    * The linux `p_mt_cond_signal` implementation will now signal all threads waiting on a given cond var
    * Made X11 keyboard registration/deregistration thread-safe
    * Added queues for unprocessed key and button events in `platform/linux/window-x11`
    * Fixed a race condition in `platform/linux/event` caused by the `SIGTERM` signal handler
