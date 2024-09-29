#include "config.h"
#include <core/util.h>
#include <gui/fonts.h>
#include <gui/menu-mgr.h>
#include <core/log.h>
#include <asset-loader/plugin.h>
#include <platform/mouse.h>
#include <platform/window.h>
#include <platform/keyboard.h>
#include <stdlib.h>
#include <string.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_error.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_video.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_pixels.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL_blendmode.h>

#define MODULE_NAME "main"

enum EXIT_CODES {
    EXIT_OK                         = EXIT_SUCCESS,
    ERR_OTHER                       = EXIT_FAILURE,
    ERR_GET_BIN_DIR                 = 2,
    ERR_INIT_SDL                    = 3,
    ERR_CREATE_WINDOW               = 4,
    ERR_CREATE_RENDERER             = 5,
    ERR_SET_RENDERER_BLENDMODE      = 6,
    ERR_INIT_KEYBOARD               = 7,
    ERR_INIT_MOUSE                  = 8,
    ERR_INIT_MENU_MANAGER           = 9,
    ERR_INIT_FONT                   = 10,
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
static struct MenuManager *MenuManager = NULL;
static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;
static struct p_window *win = NULL;
static struct p_keyboard *keyboard = NULL;
static struct p_mouse *mouse = NULL;
static struct font *sourceCodeProFont = NULL;

/* Fix linker error ('undefined reference to WinMain') when compiling for windows */
#ifdef _WIN32
int WinMain(int argc, char **argv)
#else
int main(int argc, char **argv)
#endif
{
    s_configure_log(LOG_INFO, stdout, stderr);

#ifndef CGD_BUILDTYPE_RELEASE
    if (!strcmp(argv[0], "debug"))
        s_set_log_level(LOG_DEBUG);
#endif /* CGD_BUILDTYPE_RELEASE */

    s_log_info("Initializing platform window, keyboard and mouse...");
    win = p_window_open(NULL, &(rect_t) { 0 }, P_WINDOW_TYPE_DUMMY);
    if (win == NULL)
        goto_error(ERR_INIT_KEYBOARD, "p_window_open() failed");

    keyboard = p_keyboard_init(win);
    if (keyboard == NULL)
        goto_error(ERR_INIT_KEYBOARD, "Failed to initialize the keyboard");

    mouse = p_mouse_init(win, 0);
    if (mouse == NULL)
        goto_error(ERR_INIT_KEYBOARD, "Failed to initialize the mouse");

    s_log_info("Initializing SDL");

    /* SDL initialization section */
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS))
        goto_error(ERR_INIT_SDL, "Failed to initialize SDL: %s", SDL_GetError());

    s_log_debug("Creating SDL Window...");
    window = SDL_CreateWindow(
        WINDOW_TITLE,
        WINDOW_X, WINDOW_Y, WINDOW_WIDTH, WINDOW_HEIGHT,
        WINDOW_FLAGS
    );
    if (window == NULL)
        goto_error(ERR_CREATE_WINDOW, "Failed to create SDL Window: %s", SDL_GetError());

    s_log_debug("Creating SDL Renderer...");
    renderer = SDL_CreateRenderer(window, -1, RENDERER_FLAGS);
    if (renderer == NULL)
        goto_error(ERR_CREATE_RENDERER, "Failed to create SDL Renderer: %s", SDL_GetError());

    s_log_debug("Setting SDL blend mode to BLEND");
    if (SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND))
        goto_error(ERR_SET_RENDERER_BLENDMODE, "Failed to set SDL blend mode to BLEND: %s", SDL_GetError());

    s_log_info("Initializing the GUI...");
    s_log_debug("Initializing the menu manager");
    MenuManager = menu_mgr_init(&menu_manager_cfg, renderer, keyboard, mouse);
    if (MenuManager == NULL)
        goto_error(ERR_INIT_MENU_MANAGER, "Failed to initialize the menu manager");

    s_log_debug("Initializing the font");
    sourceCodeProFont = font_init(
        "fonts/SourceCodePro-Semibold.otf",
        renderer,
        0.f, 30.f,
        FNT_CHARSET_ASCII,
        0
    );
    if (sourceCodeProFont == NULL)
        goto_error(ERR_INIT_FONT, "Failed to initialize the font");

    font_set_text_color(sourceCodeProFont, 150, 150, 150, 255);

    s_log_info("Init OK! Entering main loop...");
    /* MAIN LOOP */
    while(running)
    {
        u32 startTime = SDL_GetTicks();

        /* EVENT/INPUT HANDLING SECTION */

        /* Check for input events */
        SDL_Event event;
        while(SDL_PollEvent(&event)){
            if(event.type == SDL_QUIT){
                running = false;
            }
        }

        p_keyboard_update(keyboard);
        p_mouse_update(mouse);

        /* UPDATE SECTION */
        menu_mgr_update(MenuManager, keyboard, mouse, paused);

        if(!paused){

            /* RENDER SECTION */
            SDL_SetRenderDrawColor(renderer, u_color_arg_expand(rendererBg));
            SDL_RenderClear(renderer);

            menu_mgr_draw(MenuManager, renderer);

            SDL_RenderPresent(renderer);

        }

        Uint32 deltaTime = SDL_GetTicks() - startTime;
        if(deltaTime <= FRAME_DURATION)
            SDL_Delay(FRAME_DURATION - deltaTime);
    }

    s_log_info("Exited from the main loop, starting cleanup...");
    EXIT_CODE = EXIT_OK;

cleanup:
    if (sourceCodeProFont != NULL) font_destroy(sourceCodeProFont);
    if (MenuManager != NULL) menu_mgr_destroy(MenuManager);
    if (renderer != NULL) SDL_DestroyRenderer(renderer);
    if (window != NULL) SDL_DestroyWindow(window);
    SDL_Quit();
    if (mouse != NULL) p_mouse_destroy(mouse);
    if (keyboard != NULL) p_keyboard_destroy(keyboard);
    if (win != NULL) p_window_close(win);
    asset_unload_all_plugins();

    s_log_info("Cleanup done, Exiting with code %i.", EXIT_CODE);
    exit(EXIT_CODE);
}

