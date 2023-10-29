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

typedef int(*btn_OnClickFnPtr)(int argc, void **argv);
typedef struct {
    spr_Sprite *sprite;
    po_PressableObj button;
    btn_OnClickFnPtr onClick;
    int onClickArgc;
    void **onClickArgv;
} btn_Button;

typedef spr_SpriteConfig btn_SpriteConfig;

typedef struct {
    btn_OnClickFnPtr fn;
    int argc;
    void **argv;
} btn_OnClickConfig;

btn_Button *btn_initButton(btn_SpriteConfig *spriteCfg, btn_OnClickConfig *onClickCfg, SDL_Renderer *renderer);
void btn_updateButton(btn_Button *btn, input_Mouse *mouse);
void btn_drawButton(btn_Button *btn, SDL_Renderer *r, bool displayHitboxButton);
void btn_destroyButton(btn_Button *btn);

static const SDL_Color btn_hitboxOutlineColorNormal = { 255, 0, 0, 255 };
static const SDL_Color btn_hitboxOutlineColorPressed = { 0, 255, 0, 255 };

#endif
