#ifndef BUTTONS_H
#define BUTTONS_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_pixels.h>
#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_render.h>
#include "../user-input/keyboard.h"
#include "../user-input/mouse.h"

typedef int (btn_onClickFn) ();

typedef struct {
    SDL_Rect hitbox;
    SDL_Rect srcRect;
    SDL_Rect destRect;
    SDL_Texture* texture;
    btn_onClickFn* onClick;
    bool pressed;
    int time;
} btn_Button;

void btn_initButton(btn_Button* button, SDL_Renderer* renderer, int (*onClick) (), const SDL_Rect hitbox, const SDL_Rect srcRect, const SDL_Rect destRect, const char* textureFilePath);
void btn_updateButton(btn_Button* button, input_Mouse mouse);
void btn_drawButton(btn_Button button, SDL_Renderer* renderer, bool displayHitboxButton);
void btn_destroyButton(btn_Button* button);

int btn_onClick_printMessage();

static const SDL_Color btn_hitboxOutlineColorNormal = { 255, 0, 0, 255 };
static const SDL_Color btn_hitboxOutlineColorPressed = { 0, 255, 0, 255 };

#endif
