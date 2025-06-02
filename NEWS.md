## NEWS for 02.06.2025

* Fixed the `WM_CLASS` property not being set by `window_X11_open`
    * just what the title says
    * See `platform/linux/window-x11.c:623` (`set_window_properties`) for a more detailed explanation
