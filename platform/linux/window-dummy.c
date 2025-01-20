#include "../window.h"
#include <core/log.h>
#include <core/pixel.h>
#include <string.h>
#define P_INTERNAL_GUARD__
#include "window-dummy.h"
#undef P_INTERNAL_GUARD__

#define MODULE_NAME "window-dummy"

i32 window_dummy_init(struct window_dummy *win, const u32 flags)
{
    memset(win, 0, sizeof(struct window_dummy));

    if (flags & P_WINDOW_REQUIRE_ACCELERATED) {
        s_log_error("GPU acceleration requried, "
            "but dummy windows don't support it.");
        return 1;
    }

    return 0;
}

void window_dummy_destroy(struct window_dummy *win)
{
    memset(win, 0, sizeof(struct window_dummy));
}
