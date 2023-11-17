#include "buttons.h"
#include "../util.h"
#include "on-event.h"
#include "sprite.h"
#include <SDL2/SDL_render.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

btn_Button *btn_initButton(btn_SpriteConfig *spriteCfg, oe_OnEvent *onClick, SDL_Renderer *renderer)
{
    btn_Button *btn = malloc(sizeof(btn_Button));

    btn->sprite = spr_initSprite(spriteCfg, renderer);
    btn->button = po_createPressableObj();
    btn->onClick.fn = onClick->fn;
    btn->onClick.argc = onClick->argc;
    memcpy(btn->onClick.argv, onClick->argv, OE_ARGV_SIZE * sizeof(void*));

    return btn;
}

void btn_updateButton(btn_Button *btn, ms_Mouse *mouse)
{
    const SDL_Rect mouseRect = { mouse->x, mouse->y, 0, 0 };;
    bool hovering = u_collision(&mouseRect, &btn->sprite->hitbox);

    po_updatePressableObj(btn->button, hovering && mouse->buttonLeft->pressed);

    if(btn->button->pressed && btn->onClick.fn)
        oe_executeOnEventfn(btn->onClick);
}

void btn_drawButton(btn_Button *btn, SDL_Renderer *renderer, bool displayHitboxOutline)
{
    spr_drawSprite(btn->sprite, renderer);

    if(displayHitboxOutline){
        SDL_Color outlineColor;
        if(btn->button->pressed)
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
    po_destroyPressableObj(btn->button);

    free(btn);
    btn = NULL;
}
