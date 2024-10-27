#include "core/shapes.h"
#ifndef P_INTERNAL_GUARD__
#error This header file is internal to the cgd platform module and is not intended to be used elsewhere
#endif /* P_INTERNAL_GUARD__ */

#include "../thread.h"
#include <core/shapes.h>
#include <stdbool.h>
#ifndef WIN32_LEAN_AND_MEAN
#define WINWIN32_LEAN_AND_MEAN
#endif /* WIN32_LEAN_AND_MEAN */
#include <windows.h>
#include <windef.h>
#include <minwindef.h>

struct window_state_ro {
    volatile _Atomic const bool test;
};
struct window_state_rw {
    volatile _Atomic bool test;
};

struct p_window {
    HWND win;
    HINSTANCE instance_handle;

    struct {
        p_mt_thread_t thread;
        bool running;
    } event_dispatcher;

    struct window_state_ro state_ro;

    rect_t rect;
};
