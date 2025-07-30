#include "init.h"
#include "core/util.h"
#include "gui/menu-mgr.h"
#include "render/rctx.h"
#include <core/log.h>
#include <platform/mouse.h>
#include <platform/window.h>
#include <platform/keyboard.h>
#include <platform/platform.h>
#define CGD_GAME_CONFIG__
#include "config.h"
#undef CGD_GAME_CONFIG__

#define MODULE_NAME "init"

i32 do_platform_init(i32 argc, const char *const *argv,
    struct platform_ctx *ctx)
{
    (void) argc;
    (void) argv;
    s_log_info("Starting platform init...");

    s_log_verbose("Creating the window...");
    ctx->win = p_window_open(WINDOW_TITLE, &WINDOW_RECT, WINDOW_FLAGS);
    if (ctx->win == NULL)
        goto_error("p_window_open() failed");

    s_log_verbose("Initializing the keyboard...");
    ctx->keyboard = p_keyboard_init(ctx->win);
    if (ctx->keyboard == NULL)
        goto_error("Failed to initialize the keyboard");

    s_log_verbose("Initializing the mouse...");
    ctx->mouse = p_mouse_init(ctx->win);
    if (ctx->mouse == NULL)
        goto_error("Failed to initialize the mouse");

    ctx->running = true;
    s_log_info("Platform init OK!");
    return 0;

err:
    s_log_error("Platform init failed! Stop.");
    return 1;
}

i32 do_gui_init(struct gui_ctx *gui, const struct platform_ctx *p)
{
    s_log_info("Starting GUI init...");

    s_log_verbose("Creating the renderer...");
    gui->r = r_ctx_init(p->win, R_TYPE_SOFTWARE, 0);
    if (gui->r == NULL)
        goto_error("Failed to initialize the renderer");

    gui->mmgr = menu_mgr_init(&menu_manager_cfg,
        p->keyboard, p->mouse);
    if (gui->mmgr == NULL)
        goto_error("Failed to initialize the menu manager!");

    gui->paused = false;

    return 0;

err:
    return 1;
}

void do_platform_cleanup(struct platform_ctx *ctx)
{
    ctx->running = false;
    if (ctx->mouse != NULL) p_mouse_destroy(&ctx->mouse);
    if (ctx->keyboard != NULL) p_keyboard_destroy(&ctx->keyboard);
    if (ctx->win != NULL) p_window_close(&ctx->win);
}

void do_gui_cleanup(struct gui_ctx *gui)
{
    if (gui->mmgr != NULL) menu_mgr_destroy(&gui->mmgr);
    if (gui->r != NULL) r_ctx_destroy(&gui->r);
}
