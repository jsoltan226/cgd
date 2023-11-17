#include <SDL2/SDL_blendmode.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL_pixels.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_video.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <SDL2/SDL.h>
#include <math.h>
#include "gui/menu-mgr.h"
#include "gui/menu.h"
#include "user-input/keyboard.h"
#include "user-input/mouse.h"
#include "util.h"
#include "config.h"

/* bool running = true (moved to config.h) */
mmgr_MenuManager *MenuManager;
SDL_Window *window;
SDL_Renderer *renderer;
bool paused = false;
kb_Keyboard *keyboard;
ms_Mouse *mouse;

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

    while(running)
    {
        Uint32 startTime = SDL_GetTicks();

        /* EVENT/INPUT HANDLING SECTION */
        SDL_Event event;
        while(SDL_PollEvent(&event)){
            if(event.type == SDL_KEYUP || event.type == SDL_KEYDOWN)
                kb_updateKeyboard(keyboard);
            else if(event.type == SDL_MOUSEMOTION || 
                    event.type == SDL_MOUSEBUTTONUP || 
                    event.type == SDL_MOUSEBUTTONDOWN)
            {
                ms_updateMouse(mouse);
                /* Reset mouse when it gets out of the window */
                SDL_Rect windowRect = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };
                SDL_Rect mouseRect = { mouse->x, mouse->y, 0, 0 };
                if(!u_collision(&windowRect, &mouseRect))
                    ms_forceReleaseMouse(mouse, MS_EVERYBUTTONMASK);
            } else if(event.type == SDL_QUIT)
                running = false;

            if(kb_getKey(keyboard, KB_KEYCODE_ESCAPE)->up)
                paused = !paused;

            if(kb_getKey(keyboard, KB_KEYCODE_H)->up)
                displayButtonHitboxOutlines = !displayButtonHitboxOutlines;
        }

        /* UPDATE SECTION */
        if(!paused){
            mmgr_updateMenuManager(MenuManager, keyboard, mouse);

        /* RENDER SECTION */
            SDL_SetRenderDrawColor(renderer, rendererBg.r, rendererBg.g, rendererBg.b, rendererBg.a);
            SDL_RenderClear(renderer);

            mmgr_drawMenuManager(MenuManager, renderer, displayButtonHitboxOutlines);

            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 100);
            SDL_RenderDrawRect(renderer, &gameRect);


            SDL_RenderPresent(renderer);

            Uint32 deltaTime = SDL_GetTicks() - startTime;

            if(deltaTime < FRAME_DURATION)
                SDL_Delay(FRAME_DURATION - deltaTime);
        }
    }

    mmgr_destroyMenuManager(MenuManager);
    kb_destroyKeyboard(keyboard);
    ms_destroyMouse(mouse);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return EXIT_SUCCESS;
}

