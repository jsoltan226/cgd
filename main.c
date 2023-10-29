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
#include <SDL2/SDL_image.h>
#include <math.h>
#include "gui/menu-mgr.h"
#include "gui/menu.h"
#include "user-input/keyboard.h"
#include "user-input/mouse.h"
#include "util.h"
#include "config.h"

mmgr_MenuManager* MenuManager;
SDL_Window* window;
SDL_Renderer* renderer;
bool running = true;
bool paused = false;
kb_Keyboard *keyboard;
ms_Mouse *mouse;

int main(int argc, char** argv)
{

    SDL_Init(SDL_INIT_VIDEO);
    IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG);

    window = SDL_CreateWindow(WINDOW_TITLE, WINDOW_X, WINDOW_Y, WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_FLAGS);
    renderer = SDL_CreateRenderer(window, -1, RENDERER_FLAGS);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    keyboard = kb_initKeyboard();
    mouse = ms_initMouse();

    MenuManager = mmgr_initMenuManager((mmgr_MenuManagerConfig*)&menuManagerConfig, renderer, keyboard, mouse);


    while(running)
    {

        SDL_Event event;
        while(SDL_PollEvent(&event));

        /* EVENT/INPUT HANDLING SECTION */
        kb_updateKeyboard(keyboard);
        ms_updateMouse(mouse);

        /* Reset mouse when it gets out of the window */
        SDL_Rect windowRect = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };
        SDL_Rect mouseRect = { mouse->x, mouse->y, 0, 0 };
        if(!u_collision(&windowRect, &mouseRect))
            ms_forceReleaseMouse(mouse, MS_EVERYBUTTONMASK);

        if(event.type == SDL_QUIT || kb_getKey(keyboard, KB_KEYCODE_Q)->up || mouse->buttonMiddle->up)
            running = false;

        if(kb_getKey(keyboard, KB_KEYCODE_ESCAPE)->up)
            paused = !paused;

        if(kb_getKey(keyboard, KB_KEYCODE_H)->up)
            displayButtonHitboxOutlines = !displayButtonHitboxOutlines;

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

            SDL_Delay(FRAME_DURATION);
        }
    }

    ms_destroyMouse(mouse);
    mmgr_destroyMenuManager(MenuManager);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return EXIT_SUCCESS;
}

