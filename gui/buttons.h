#pragma once
#ifndef BUTTONS_H
#define BUTTONS_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_pixels.h>
#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_render.h>
#include "../user-input/keyboard.h"
#include "../user-input/mouse.h"
#include "sprite.h"

typedef struct {
    spr_Sprite* sprite;
    po_PressableObj button;
} btn_Button;

btn_Button* btn_initButton(spr_SpriteConfig* spriteCfg, SDL_Renderer* renderer);
void btn_updateButton(btn_Button* btn, input_Mouse mouse);
void btn_drawButton(btn_Button* btn, SDL_Renderer* r, bool displayHitboxButton);
void btn_destroyButton(btn_Button* btn);

static const SDL_Color btn_hitboxOutlineColorNormal = { 255, 0, 0, 255 };
static const SDL_Color btn_hitboxOutlineColorPressed = { 0, 255, 0, 255 };

#endif
