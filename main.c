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

static mmgr_MenuManager* MenuManager;
static SDL_Window* window;
static SDL_Renderer* renderer;
static bool running = true;
static bool paused = false;
static input_Keyboard keyboard;
static input_Mouse mouse;

int main(int argc, char** argv)
{

    SDL_Init(SDL_INIT_VIDEO);
    IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG);

    window = SDL_CreateWindow(WINDOW_TITLE, WINDOW_X, WINDOW_Y, WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_FLAGS);
    renderer = SDL_CreateRenderer(window, -1, RENDERER_FLAGS);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    MenuManager = mmgr_initMenuManager((mmgr_MenuManagerConfig*)&menuManagerConfig, renderer);

    input_initKeyboard(keyboard);


    while(running)
    {

        SDL_Event event;
        while(SDL_PollEvent(&event));

        /* EVENT/INPUT HANDLING SECTION */
        input_updateKeyboard(keyboard);
        input_updateMouse(&mouse);

        if(event.type == SDL_QUIT || keyboard[INPUT_KEYCODE_Q].key.pressed || mouse.buttonMiddle.pressed)
            running = false;

        if(keyboard[INPUT_KEYCODE_ESCAPE].key.up)
            paused = !paused;

        if(keyboard[INPUT_KEYCODE_H].key.up)
            displayButtonHitboxOutlines = !displayButtonHitboxOutlines;

        /* UPDATE SECTION */
        if(!paused){
            mmgr_updateMenuManager(MenuManager, keyboard, mouse);

            if(keyboard[INPUT_KEYCODE_P].key.down && MenuManager->inMenuDepth == 1){
                mmgr_switchMenu(MenuManager, MenuManager->fullMenuList[0]);
                MenuManager->inMenuDepth = 0;
            }

            if(MenuManager->fullMenuList[0]->buttons[MAIN_MENU_BUTTON_TESTBUTTON]->button.down && MenuManager->inMenuDepth == 0){
                mmgr_switchMenu(MenuManager, MenuManager->fullMenuList[1]);
                MenuManager->fullMenuList[0]->buttons[MAIN_MENU_BUTTON_TESTBUTTON]->button.down = false;
                MenuManager->inMenuDepth = 1;
            }

        
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

    mmgr_destroyMenuManager(MenuManager);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return EXIT_SUCCESS;
}

