#include "buttons.h"
#include "../util.h"
#include "sprite.h"
#include <SDL2/SDL_render.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

btn_Button *btn_initButton(btn_SpriteConfig *spriteCfg, btn_OnClickConfig *onClickCfg, SDL_Renderer *renderer)
{
    btn_Button *btn = malloc(sizeof(btn_Button));

    btn->sprite = spr_initSprite(spriteCfg, renderer);
    btn->button = po_createPressableObj();
    btn->onClick = onClickCfg->fn;
    btn->onClickArgc = onClickCfg->argc;
    btn->onClickArgv = onClickCfg->argv;

    return btn;
}

void btn_updateButton(btn_Button *btn, input_Mouse *mouse)
{
    const SDL_Rect mouseRect = { mouse->x, mouse->y, 0, 0 };
    bool hovering = u_collision(mouseRect, btn->sprite->hitbox);

    po_updatePressableObj(&btn->button, hovering && mouse->buttonLeft.pressed);

    if(btn->button.pressed && btn->onClick)
        btn->onClick(btn->onClickArgc, btn->onClickArgv);
}

void btn_drawButton(btn_Button *btn, SDL_Renderer *renderer, bool displayHitboxOutline)
{
    spr_drawSprite(btn->sprite, renderer);

    if(displayHitboxOutline){
        SDL_Color outlineColor;
        if(btn->button.pressed)
            outlineColor = btn_hitboxOutlineColorPressed;
        else
            outlineColor = btn_hitboxOutlineColorNormal;

        SDL_SetRenderDrawColor(renderer, outlineColor.r, outlineColor.g, outlineColor.b, outlineColor.a);
    }
    SDL_RenderDrawRect(renderer, &btn->sprite->hitbox);
}

void btn_destroyButton(btn_Button *btn)
{
    spr_destroySprite(btn->sprite);
    if(btn->onClickArgv)
        fputs("WARNING (from btn_destroyButton): btn->onClickArgv was not set to NULL!\n", stderr);

    free(btn);
    btn = NULL;
}
