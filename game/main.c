#include "config.h"
#include "core/shapes.h"
#include <core/util.h>
#include <gui/menu-mgr.h>
#include <core/log.h>
#include <asset-loader/plugin.h>
#include <render/rctx.h>
#include <render/rect.h>
#include <platform/time.h>
#include <platform/event.h>
#include <platform/mouse.h>
#include <platform/window.h>
#include <platform/keyboard.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MODULE_NAME "main"

enum EXIT_CODES {
    EXIT_OK                         = 0,
    ERR_OTHER                       = 1,
    ERR_CREATE_WINDOW               = 2,
    ERR_CREATE_RENDERER             = 3,
    ERR_INIT_KEYBOARD               = 4,
    ERR_INIT_MOUSE                  = 5,
    ERR_INIT_MENU_MANAGER           = 6,
    ERR_MAX
};
static enum EXIT_CODES EXIT_CODE;
#undef goto_error
#define goto_error(code, ...) do {          \
    EXIT_CODE = code;                       \
    s_log_error(__VA_ARGS__);               \
    goto cleanup;                           \
} while (0);

/* bool running = true (moved to config.h) */
static FILE *log_file = NULL;
static struct MenuManager *MenuManager = NULL;
static struct p_window *win = NULL;
static struct r_ctx *rctx = NULL;
static struct p_keyboard *keyboard = NULL;
static struct p_mouse *mouse = NULL;

static void cleanup_log(void);

int cgd_main(int argc, char **argv)
{
    log_file = fopen("log.txt", "wb");
    if (atexit(cleanup_log)) {
        s_log_error("Failed to atexit() the log cleanup function.");
        return EXIT_FAILURE;
    }
    s_configure_log(LOG_DEBUG, log_file, log_file);
    //s_configure_log(LOG_INFO, log_file, log_file);

#ifndef CGD_BUILDTYPE_RELEASE
    if (!strcmp(argv[0], "debug"))
        s_set_log_level(LOG_DEBUG);
#endif /* CGD_BUILDTYPE_RELEASE */

    s_log_info("Initializing platform window, keyboard and mouse...");
    win = p_window_open(WINDOW_TITLE, &WINDOW_RECT, WINDOW_FLAGS);
    if (win == NULL)
        goto_error(ERR_CREATE_WINDOW, "p_window_open() failed");

    rctx = r_ctx_init(win, R_TYPE_SOFTWARE, 0);
    if (rctx == NULL)
        goto_error(ERR_CREATE_RENDERER, "Failed to initialize the renderer");

    keyboard = p_keyboard_init(win);
    if (keyboard == NULL)
        goto_error(ERR_INIT_KEYBOARD, "Failed to initialize the keyboard");

    mouse = p_mouse_init(win, 0);
    if (mouse == NULL)
        goto_error(ERR_INIT_MOUSE, "Failed to initialize the mouse");

    s_log_info("Initializing the GUI...");
    s_log_debug("Initializing the menu manager");
    MenuManager = menu_mgr_init(&menu_manager_cfg, rctx, keyboard, mouse);
    if (MenuManager == NULL)
        goto_error(ERR_INIT_MENU_MANAGER, "Failed to initialize the menu manager");

    s_log_info("Init OK! Entering main loop...");
    /* MAIN LOOP */
    while(running) {
        p_time_t start_time;
        p_time(&start_time);

        /* EVENT/INPUT HANDLING SECTION */

        /* Check for events */
        struct p_event ev;
        while(p_event_poll(&ev)) {
            if(ev.type == P_EVENT_QUIT){
                running = false;
                goto main_loop_breakout;
            }
        }

        p_keyboard_update(keyboard);
        p_mouse_update(mouse);

        /* UPDATE SECTION */
        menu_mgr_update(MenuManager, keyboard, mouse, paused);

        if(!paused){

            /* RENDER SECTION */
            r_reset(rctx);
            menu_mgr_draw(MenuManager, rctx);
            r_ctx_set_color(rctx, (color_RGBA32_t) { 200, 200, 200, 255 });
            r_draw_rect(rctx, rect_arg_expand(gameRect));
            r_flush(rctx);
        }

        i64 delta_time = p_time_delta_us(&start_time);
        if(delta_time <= FRAME_DURATION_us)
            p_time_usleep(FRAME_DURATION_us - delta_time);
    }
main_loop_breakout:

    s_log_info("Exited from the main loop, starting cleanup...");
    EXIT_CODE = EXIT_OK;

cleanup:
    if (MenuManager != NULL) menu_mgr_destroy(&MenuManager);
    if (mouse != NULL) p_mouse_destroy(&mouse);
    if (keyboard != NULL) p_keyboard_destroy(&keyboard);
    if (rctx != NULL) r_ctx_destroy(&rctx);
    if (win != NULL) p_window_close(&win);

    s_log_info("Cleanup done, Exiting with code %i.", EXIT_CODE);
    return EXIT_CODE;
}

static void cleanup_log(void)
{
    if (log_file != NULL) {
        fclose(log_file);
        log_file = NULL;
    }
    s_set_log_out_filep(stdout);
    s_set_log_err_filep(stderr);
}
