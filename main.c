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
#include "main-menu/parallax-bg.h"
#include "main-menu/buttons.h"
#include "user-input/keyboard.h"
#include "user-input/mouse.h"
#include "gutils.h"
#include "config.h"

static bg_ParallaxBG mainBG;
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


    bg_initBG(&mainBG, renderer, mainBGLayerImageFilePaths, mainBGLayerSpeeds, mainBGLayerCount);
    input_initKeyboard(keyboard);

    while(running)
    {
        /* EVENT/INPUT HANDLING SECTION */
        input_updateKeyboard(keyboard);
        input_updateMouse(&mouse);

        SDL_Event event;
        while(SDL_PollEvent(&event)){
            if(event.type == SDL_QUIT || keyboard[INPUT_KEYCODE_Q].pressed || mouse.buttonMiddle.pressed)
                running = false;

            if(keyboard[INPUT_KEYCODE_ESCAPE].up)
                paused = !paused;
        };

        /* UPDATE SECTION */
        if(!paused){
            bg_updateBG(&mainBG);
            if(mouse.buttonLeft.down)
                mainBG.layers[1].speed = max(0, mainBG.layers[1].speed - 1);
            if(mouse.buttonRight.down)
                mainBG.layers[1].speed++;
            if(keyboard[INPUT_KEYCODE_S].down)
                mainBG.layers[1].speed = 0;
        
        /* RENDER SECTION */
            SDL_SetRenderDrawColor(renderer, rendererBg.r, rendererBg.g, rendererBg.b, rendererBg.a);
            SDL_RenderClear(renderer);

            bg_drawBG(&mainBG, renderer);

            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 100);
            SDL_RenderDrawRect(renderer, &gameRect);

            SDL_RenderPresent(renderer);

            SDL_Delay(FRAME_DURATION);
        }
    }

    bg_destroyBG(&mainBG);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return EXIT_SUCCESS;
}

