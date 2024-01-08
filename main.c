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
#include <cgd/util/util.h>
#include <cgd/util/ansi-esc-sequences.h>
#include <cgd/user-input/keyboard.h>
#include <cgd/user-input/mouse.h>
#include <cgd/gui/fonts.h>
#include <cgd/gui/menu-mgr.h>
#include "config.h"

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
enum EXIT_CODES EXIT_CODE;

/* bool running = true (moved to config.h) */
mmgr_MenuManager *MenuManager;
SDL_Window *window;
SDL_Renderer *renderer;
kb_Keyboard *keyboard;
ms_Mouse *mouse;
fnt_Font *sourceCodeProFont;

/* Fix linker error ('undefined reference to WinMain') when compiling for windows */
#ifdef _WIN32
int WinMain(int argc, char **argv)
#else
int main(int argc, char **argv)
#endif
{
    /* SDL initialization section */
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
        EXIT_CODE = ERR_INIT_SDL;
        goto err;
    }

    window = SDL_CreateWindow(WINDOW_TITLE, WINDOW_X, WINDOW_Y, WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_FLAGS);
    if (window == NULL) {
        EXIT_CODE = ERR_CREATE_WINDOW;
        goto err;
    }

    renderer = SDL_CreateRenderer(window, -1, RENDERER_FLAGS);
    if (renderer == NULL) {
        EXIT_CODE = ERR_CREATE_RENDERER;
        goto err;
    }

    if ( SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND)) {
        EXIT_CODE = ERR_SET_RENDERER_BLENDMODE;
        goto err;
    }

    /* Initialize the engine structs */
    keyboard = kb_initKeyboard();
    if (keyboard == NULL) {
        EXIT_CODE = ERR_INIT_KEYBOARD;
        goto err;
    }

    mouse = ms_initMouse();
    if (mouse == NULL) {
        EXIT_CODE = ERR_INIT_MOUSE;
        goto err;
    }

    MenuManager = mmgr_initMenuManager((mmgr_MenuManagerConfig*)&menuManagerConfig, renderer, keyboard, mouse);
    if (MenuManager == NULL) {
        EXIT_CODE = ERR_INIT_MENU_MANAGER;
        goto err;
    }

    sourceCodeProFont = fnt_initFont("/fonts/SourceCodePro-Semibold.otf", renderer, 0.f, 30.f, FNT_CHARSET_ASCII, 0);
    if (sourceCodeProFont == NULL) {
        EXIT_CODE = ERR_INIT_FONT;
        goto err;
    }
    fnt_setTextColor(sourceCodeProFont, 150, 150, 150, 255);

    /* MAIN LOOP */
    while(running)
    {
        Uint32 startTime = SDL_GetTicks();

        /* EVENT/INPUT HANDLING SECTION */

        /* The keyboard and mouse structs need updating every game tick. 
         * They use SDL_Get(..)State, not SDL_PollEvent. */
        kb_updateKeyboard(keyboard);
        ms_updateMouse(mouse);

        /* Check for input events */
        SDL_Event event;
        while(SDL_PollEvent(&event)){
            if(event.type == SDL_MOUSEMOTION || event.type == SDL_MOUSEBUTTONUP || event.type == SDL_MOUSEBUTTONDOWN){

                /* Reset mouse when it gets out of the window */
                SDL_Rect windowRect = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };
                SDL_Rect mouseRect = { mouse->x, mouse->y, 0, 0 };

                if(!u_collision(&windowRect, &mouseRect))
                    ms_forceReleaseMouse(mouse, MS_EVERYBUTTONMASK);

            } else if(event.type == SDL_QUIT){
                running = false; 
            }
        }

        /* UPDATE SECTION */
        mmgr_updateMenuManager(MenuManager, keyboard, mouse, paused);

        if(!paused){

        /* RENDER SECTION */
            SDL_SetRenderDrawColor(renderer, rendererBg.r, rendererBg.g, rendererBg.b, rendererBg.a);
            SDL_RenderClear(renderer);

            mmgr_drawMenuManager(MenuManager, renderer, displayButtonHitboxOutlines);

            if(kb_getKey(keyboard, KB_KEYCODE_DIGIT1)->up)
                sourceCodeProFont->flags ^= FNT_FLAG_DISPLAY_TEXT_RECTS;
            if(kb_getKey(keyboard, KB_KEYCODE_DIGIT2)->up)
                sourceCodeProFont->flags ^= FNT_FLAG_DISPLAY_GLYPH_RECTS;
            if(kb_getKey(keyboard, KB_KEYCODE_DIGIT3)->up)
                sourceCodeProFont->flags ^= FNT_FLAG_DISPLAY_CHAR_RECTS;

            fnt_Vector2D textPos = { .x = mouse->x, .y = mouse->y };

            fnt_renderText(sourceCodeProFont, renderer,  &textPos, 
                    "Working text!\nsourceCodeProFont->flags:\v%i%i%i\t(%i)", 
                        (sourceCodeProFont->flags & 4) >> 2,
                        (sourceCodeProFont->flags & 2) >> 1,
                         sourceCodeProFont->flags & 1,
                         sourceCodeProFont->flags);

            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 100);
            SDL_RenderDrawRect(renderer, &gameRect);

            SDL_RenderPresent(renderer);

            Uint32 deltaTime = SDL_GetTicks() - startTime;

            if(deltaTime <= FRAME_DURATION)
                SDL_Delay(FRAME_DURATION - deltaTime);
        }
    }

    /* Cleanup section */
    fnt_destroyFont(sourceCodeProFont);
    mmgr_destroyMenuManager(MenuManager);
    kb_destroyKeyboard(keyboard);
    ms_destroyMouse(mouse);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return EXIT_SUCCESS;

err:
    /* Make sure to print the error message BEFORE attempting any shenanigans with freeing the resources */
    ;
    const char *errorMessages[ERR_MAX] = {
        [EXIT_OK]                       = "Everything is supposed to be OK, and yet the 'err' label is used. The developer is an idiot",
        [ERR_OTHER]                     = "An unknown error occured. This should never happen (i. e. the developer fucked up)\n",
        [ERR_GET_BIN_DIR]               = "Failed to get the directory in which the executable is located.",
        [ERR_INIT_SDL]                  = "Failed to initialize SDL. Details: ",
        [ERR_CREATE_WINDOW]             = "Failed to create window. Details: ",
        [ERR_CREATE_RENDERER]           = "Failed to create rendering context. Details: ",
        [ERR_SET_RENDERER_BLENDMODE]    = "Failed to enable texture blending in the rendering context. Details: ",
        [ERR_INIT_KEYBOARD]             = "Failed to init the keyboard",
        [ERR_INIT_MOUSE]                = "Failed to init the mouse",
        [ERR_INIT_MENU_MANAGER]         = "Failed to initialize the menu manager. Good luck finding out why..",
        [ERR_INIT_FONT]                 = "Failed to init the font",
    };
    if (EXIT_CODE >= EXIT_OK && EXIT_CODE < ERR_MAX) {
        u_error(es_BOLD es_RED "ERROR" es_COLOR_RESET ": %s %s.\n",
                errorMessages[EXIT_CODE], SDL_GetError());
    } else {
        u_error(errorMessages[ERR_OTHER]);
    }

    /* Utilize fall-through behaviour to free, based on the exit code, the resources that had been allocated up to the point when the error occured */
    switch (EXIT_CODE) {
		case ERR_INIT_FONT: mmgr_destroyMenuManager(MenuManager);
		case ERR_INIT_MENU_MANAGER: ms_destroyMouse(mouse);
		case ERR_INIT_MOUSE: kb_destroyKeyboard(keyboard);
		case ERR_INIT_KEYBOARD:
        case ERR_SET_RENDERER_BLENDMODE: SDL_DestroyRenderer(renderer);
        case ERR_CREATE_RENDERER: SDL_DestroyWindow(window);
		case ERR_CREATE_WINDOW: SDL_Quit();
        case ERR_INIT_SDL:
        case ERR_GET_BIN_DIR:
            break;
        default: case ERR_OTHER: case ERR_MAX:
            break;
        case EXIT_OK:
            break;
    }

    exit(EXIT_CODE);
}

