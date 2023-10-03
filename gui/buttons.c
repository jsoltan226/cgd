#include "buttons.h"
#include "../util.h"
#include "sprite.h"
#include <SDL2/SDL_render.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

btn_Button* btn_initButton(spr_SpriteConfig* spriteCfg, SDL_Renderer* renderer)
{
    btn_Button* btn = malloc(sizeof(btn_Button));
    assert(btn != NULL);

    btn->sprite = spr_initSprite(spriteCfg, renderer);
    btn->button = po_createPressableObj();

    return btn;
}

void btn_updateButton(btn_Button* btn, input_Mouse mouse)
{
    const SDL_Rect mouseRect = { mouse.x, mouse.y, 0, 0 };
    bool hovering = u_collision(mouseRect, btn->sprite->hitbox);

    po_updatePressableObj(&btn->button, hovering && mouse.buttonLeft.pressed);
}

void btn_drawButton(btn_Button* b, SDL_Renderer* renderer, bool displayHitboxOutline)
{
    spr_drawSprite(b->sprite, renderer);

    if(displayHitboxOutline){
        SDL_Color outlineColor;
        if(b->button.pressed)
            outlineColor = btn_hitboxOutlineColorPressed;
        else
            outlineColor = btn_hitboxOutlineColorNormal;

        SDL_SetRenderDrawColor(renderer, outlineColor.r, outlineColor.g, outlineColor.b, outlineColor.a);
    }
    SDL_RenderDrawRect(renderer, &b->sprite->hitbox);
}

void btn_destroyButton(btn_Button* btn)
{
    spr_destroySprite(btn->sprite);

    free(btn);
    btn = NULL;
}
