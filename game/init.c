#include "init.h"
#include "config.h"
#include "core/util.h"
#include "gui/menu-mgr.h"
#include "render/rctx.h"
#include <core/log.h>
#include <platform/mouse.h>
#include <platform/window.h>
#include <platform/keyboard.h>
#include <platform/platform.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MODULE_NAME "init"

static i32 setup_log(FILE **out_log_fp_p, FILE **err_log_fp_p);
static void cleanup_log(void);

i32 do_platform_init(i32 argc, const char *const *argv,
    struct platform_ctx *ctx)
{

    /* Set up logging */
    if (setup_log(&ctx->out_log_file, &ctx->err_log_file))
        goto err;

    s_log_info("Log setup OK. Starting platform init...");

#ifndef CGD_BUILDTYPE_RELEASE
    if (!strcmp(argv[0], "debug"))
        s_set_log_level(LOG_DEBUG);
#endif /* CGD_BUILDTYPE_RELEASE */

    s_log_debug("Creating the window...");
    ctx->win = p_window_open(WINDOW_TITLE, &WINDOW_RECT, WINDOW_FLAGS);
    if (ctx->win == NULL)
        goto_error("p_window_open() failed");

    s_log_debug("Initializing the keyboard...");
    ctx->keyboard = p_keyboard_init(ctx->win);
    if (ctx->keyboard == NULL)
        goto_error("Failed to initialize the keyboard");

    s_log_debug("Initializing the mouse...");
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

    s_log_debug("Creating the renderer...");
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

    /* These get closed in the calls to `s_close_log_..._file()`
     * in `cleanup_log`, which runs automatically at program exit */
    ctx->out_log_file = NULL;
    ctx->err_log_file = NULL;
}

void do_gui_cleanup(struct gui_ctx *gui)
{
    if (gui->mmgr != NULL) menu_mgr_destroy(&gui->mmgr);
    if (gui->r != NULL) r_ctx_destroy(&gui->r);
}

static i32 setup_log(FILE **out_log_fp_p, FILE **err_log_fp_p)
{
    s_set_log_out_filep(stdout);
    s_set_log_err_filep(stderr);
    *out_log_fp_p = NULL;
    *err_log_fp_p = NULL;

#if (PLATFORM == PLATFORM_WINDOWS)
#include <errno.h>

    FILE *tmp_log_fp = fopen(LOG_FILEPATH, "wb");
    if (tmp_log_fp == NULL) {
        s_log_error("Failed to open log file \"%s\": %s",
            LOG_FILEPATH, strerror(errno));
        return 1;
    }

    if (atexit(cleanup_log)) {
        s_log_error("Failed to atexit() the log cleanup function: %s",
            strerror(errno));
        fclose(tmp_log_fp);
        return 1;
    }

    *out_log_fp_p = tmp_log_fp;
    *err_log_fp_p = tmp_log_fp;
#else
    (void) cleanup_log; /* Make compiler happy */
    *out_log_fp_p = stdout;
    *err_log_fp_p = stderr;
#endif /* PLATFORM */

    s_set_log_out_filep(*out_log_fp_p);
    s_set_log_err_filep(*err_log_fp_p);

    return 0;
}

static void cleanup_log(void)
{
    s_close_out_log_fp();
    s_close_err_log_fp();
}
