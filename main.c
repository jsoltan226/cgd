#include <SDL2/SDL_blendmode.h>
#include <SDL2/SDL_error.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL_pixels.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_video.h>
#include <SDL2/SDL.h>
#include <stdlib.h>
#include <string.h>
#include "core/util.h"
#include "input/keyboard.h"
#include "input/mouse.h"
#include "gui/fonts.h"
#include "gui/menu-mgr.h"
#include "config.h"
#include "core/log.h"

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
#define goto_error(code, message...) do {   \
    EXIT_CODE = code;                       \
    s_log_error("main", message);           \
    goto cleanup;                           \
} while (0);

/* bool running = true (moved to config.h) */
mmgr_MenuManager *MenuManager;
SDL_Window *window;
SDL_Renderer *renderer;
struct mouse *mouse;
struct keyboard *keyboard;
struct font *sourceCodeProFont;

/* Fix linker error ('undefined reference to WinMain') when compiling for windows */
#ifdef _WIN32
int WinMain(int argc, char **argv)
#else
int main(int argc, char **argv)
#endif
{
    s_set_log_out_filep(stdout);
    s_set_log_err_filep(stderr);
    s_set_user_fault(NO_USER_FAULT);
#ifndef NDEBUG
    if (!strcmp(argv[0], "debug"))
        s_set_log_level(LOG_DEBUG);
    else
        s_set_log_level(LOG_INFO);
#else
    s_set_log_level(LOG_INFO);
#endif /* NDEBUG */

    s_log_info("main", "Initializing SDL");

    /* SDL initialization section */
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS))
        goto_error(ERR_INIT_SDL, "Failed to initialize SDL: %s", SDL_GetError());

    s_log_debug("main", "Creating SDL Window...");
    window = SDL_CreateWindow(WINDOW_TITLE, WINDOW_X, WINDOW_Y, WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_FLAGS);
    if (window == NULL)
        goto_error(ERR_CREATE_WINDOW, "Failed to create SDL Window: %s", SDL_GetError());

    s_log_debug("main", "Creating SDL Renderer...");
    renderer = SDL_CreateRenderer(window, -1, RENDERER_FLAGS);
    if (renderer == NULL)
        goto_error(ERR_CREATE_RENDERER, "Failed to create SDL Renderer: %s", SDL_GetError());

    s_log_debug("main", "Setting SDL blend mode to BLEND");
    if (SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND))
        goto_error(ERR_SET_RENDERER_BLENDMODE, "Failed to set SDL blend mode to BLEND: %s", SDL_GetError());

    s_log_info("main", "Initializing the keyboard and mouse...");

    /* Initialize the engine structs */
    s_log_debug("main", "Initializing the keyboard");
    keyboard = keyboard_init();
    if (keyboard == NULL)
        goto_error(ERR_INIT_KEYBOARD, "Failed to initialize the keyboard!");

    s_log_debug("main", "Initializing the mouse");
    mouse = mouse_init();
    if (mouse == NULL)
        goto_error(ERR_INIT_MOUSE, "Failed to initialize the mouse!");

    s_log_info("main", "Initializing the GUI...");
    s_log_debug("main", "Initializing the menu manager");
    MenuManager = mmgr_initMenuManager((mmgr_MenuManagerConfig*)&menuManagerConfig, renderer, keyboard, mouse);
    if (MenuManager == NULL)
        goto_error(ERR_INIT_MENU_MANAGER, "Failed to initialize the menu manager");

    s_log_debug("main", "Initializing the font");
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

    s_log_info("main", "Init OK! Entering main loop...");
    /* MAIN LOOP */
    while(running)
    {
        u32 startTime = SDL_GetTicks();

        /* EVENT/INPUT HANDLING SECTION */

        keyboard_update(keyboard);
        mouse_update(mouse);

        /* Check for input events */
        SDL_Event event;
        while(SDL_PollEvent(&event)){
            if(event.type == SDL_QUIT){
                running = false;
            }
        }

        /* UPDATE SECTION */
        mmgr_updateMenuManager(MenuManager, keyboard, mouse, paused);

        if(!paused){

        /* RENDER SECTION */
            SDL_SetRenderDrawColor(renderer, u_color_arg_expand(rendererBg));
            SDL_RenderClear(renderer);

            mmgr_drawMenuManager(MenuManager, renderer);

            /*
            if(kb_getKey(keyboard, KB_KEYCODE_DIGIT1).up)
                sourceCodeProFont->flags ^= FNT_FLAG_DISPLAY_TEXT_RECTS;
            if(kb_getKey(keyboard, KB_KEYCODE_DIGIT2).up)
                sourceCodeProFont->flags ^= FNT_FLAG_DISPLAY_GLYPH_RECTS;
            if(kb_getKey(keyboard, KB_KEYCODE_DIGIT3).up)
                sourceCodeProFont->flags ^= FNT_FLAG_DISPLAY_CHAR_RECTS;

            vec2d_t textPos = { .x = mouse->x, .y = mouse->y };

            font_draw_text(sourceCodeProFont, renderer,  &textPos,
                    "Working text!\nsourceCodeProFont->flags:\v%i%i%i\t(%i)",
                        (sourceCodeProFont->flags & 4) >> 2,
                        (sourceCodeProFont->flags & 2) >> 1,
                         sourceCodeProFont->flags & 1,
                         sourceCodeProFont->flags);

            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 100);
            SDL_RenderDrawRect(renderer, &gameRect);
            */

            /*
            SDL_Rect mouse_rect = { mouse->x - 10, mouse->y - 10, 20, 20 };
            if (mouse->buttons[MOUSE_BUTTON_LEFT].pressed)
                SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
            else
                SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);

            SDL_RenderFillRect(renderer, &mouse_rect);
            */

            SDL_RenderPresent(renderer);

            Uint32 deltaTime = SDL_GetTicks() - startTime;

            if(deltaTime <= FRAME_DURATION)
                SDL_Delay(FRAME_DURATION - deltaTime);
        }
    }

    s_log_info("main", "Exited from the main loop, starting cleanup...");
    EXIT_CODE = EXIT_OK;

cleanup:
    if (sourceCodeProFont != NULL) font_destroy(sourceCodeProFont);
    if (MenuManager != NULL) mmgr_destroyMenuManager(MenuManager);
    if (keyboard != NULL) keyboard_destroy(keyboard);
    if (mouse != NULL) mouse_destroy(mouse);
    if (renderer != NULL) SDL_DestroyRenderer(renderer);
    if (window != NULL) SDL_DestroyWindow(window);
    SDL_Quit();

    s_log_info("main", "Cleanup done, Exiting with code %i.", EXIT_CODE);
    exit(EXIT_CODE);
}

