#include <SDL2/SDL_blendmode.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL_pixels.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_video.h>
#include <stdlib.h>
#include <SDL2/SDL.h>
#include "gui/menu-mgr.h"
#include "user-input/keyboard.h"
#include "user-input/mouse.h"
#include "util.h"
#include "config.h"
#include "gui/fonts.h"

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
    SDL_Init(SDL_INIT_VIDEO);

    window = SDL_CreateWindow(WINDOW_TITLE, WINDOW_X, WINDOW_Y, WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_FLAGS);
    renderer = SDL_CreateRenderer(window, -1, RENDERER_FLAGS);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    keyboard = kb_initKeyboard();
    mouse = ms_initMouse();

    MenuManager = mmgr_initMenuManager((mmgr_MenuManagerConfig*)&menuManagerConfig, renderer, keyboard, mouse);
    sourceCodeProFont = fnt_initFont("assets/fonts/SourceCodePro-Semibold.otf", renderer, 0.f, 30.f, FNT_CHARSET_ASCII, 0);

    while(running)
    {
        Uint32 startTime = SDL_GetTicks();

        /* EVENT/INPUT HANDLING SECTION */

        /* The keyboard and mouse structs need updating every game tick. They use SDL_Get(..)State, not SDL_PollEvent. */
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

            fnt_Vector2D textPos = { .x = mouse->x, .y = mouse->y };
            fnt_setTextColor(sourceCodeProFont, 0, 0, 0, 255);

            if(kb_getKey(keyboard, KB_KEYCODE_DIGIT1)->up)
                sourceCodeProFont->flags ^= FNT_FLAG_DISPLAY_TEXT_RECTS;
            if(kb_getKey(keyboard, KB_KEYCODE_DIGIT2)->up)
                sourceCodeProFont->flags ^= FNT_FLAG_DISPLAY_GLYPH_RECTS;
            if(kb_getKey(keyboard, KB_KEYCODE_DIGIT3)->up)
                sourceCodeProFont->flags ^= FNT_FLAG_DISPLAY_CHAR_RECTS;

            fnt_drawText(sourceCodeProFont, renderer,  &textPos, "Working text!\nsourceCodeProFont->flags:\v%i%i%i", (sourceCodeProFont->flags & 4) >> 2, (sourceCodeProFont->flags & 2) >> 1, sourceCodeProFont->flags & 1);

            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 100);
            SDL_RenderDrawRect(renderer, &gameRect);

            SDL_RenderPresent(renderer);

            Uint32 deltaTime = SDL_GetTicks() - startTime;

            if(deltaTime <= FRAME_DURATION)
                SDL_Delay(FRAME_DURATION - deltaTime);
        }
    }

    fnt_destroyFont(sourceCodeProFont);
    mmgr_destroyMenuManager(MenuManager);
    kb_destroyKeyboard(keyboard);
    ms_destroyMouse(mouse);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return EXIT_SUCCESS;
}

