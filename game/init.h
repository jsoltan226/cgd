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

struct main_ctx {
    struct p_window *win;
    struct r_ctx *r;

    struct p_keyboard *keyboard;
    struct p_mouse *mouse;

    bool running, paused;

    FILE *out_log_file, *err_log_file;
};

struct gui_ctx {
    struct MenuManager *mmgr;
};

i32 do_early_init(i32 argc, const char *const *argv, struct main_ctx *ctx);
i32 do_gui_init(struct gui_ctx *gui, const struct main_ctx *main_ctx);

void do_gui_cleanup(struct gui_ctx *gui);
void do_main_cleanup(struct main_ctx *ctx);

#endif /* INIT_H_ */
