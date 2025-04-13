#ifndef INIT_H_
#define INIT_H_

#include "gui/menu-mgr.h"
#include <core/int.h>
#include <platform/mouse.h>
#include <platform/window.h>
#include <platform/keyboard.h>
#include <render/rctx.h>
#include <stdio.h>
#include <stdbool.h>

struct platform_ctx {
    struct p_window *win;
    struct p_keyboard *keyboard;
    struct p_mouse *mouse;

    bool running;
};

struct gui_ctx {
    struct r_ctx *r;
    struct MenuManager *mmgr;
    bool paused;
};

i32 do_platform_init(i32 argc, const char *const *argv,
    struct platform_ctx *ctx);
i32 do_gui_init(struct gui_ctx *gui, const struct platform_ctx *p);

void do_gui_cleanup(struct gui_ctx *gui);
void do_platform_cleanup(struct platform_ctx *p_ctx);

#endif /* INIT_H_ */
