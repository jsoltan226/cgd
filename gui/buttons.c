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
    /* Allocate space for the button struct */
    btn_Button *btn = malloc(sizeof(btn_Button));
    assert(btn != NULL);

    /* Initialize the sprite member */
    btn->sprite = spr_initSprite(spriteCfg, renderer);
    assert(btn->sprite != NULL);

    /* Initialize the button (pressabel object) member */
    btn->button = po_createPressableObj();
    assert(btn->button != NULL);

    /* Copy over info given in the 'onClick' arg */
    btn->onClick.fn = onClick->fn;
    btn->onClick.argc = onClick->argc;
    assert( memcpy(btn->onClick.argv, onClick->argv, OE_ARGV_SIZE * sizeof(void*)) );

    return btn;
}

void btn_updateButton(btn_Button *btn, ms_Mouse *mouse)
{
    /* Check whether the mouse is hovering over the button ... */
    const SDL_Rect mouseRect = { mouse->x, mouse->y, 0, 0 };;
    bool hovering = u_collision(&mouseRect, &btn->sprite->hitbox);

    /* And given that info, update the button (Pressable Object) member */
    po_updatePressableObj(btn->button, hovering && mouse->buttonLeft->down);

    /* If the button is pressed and the onClick function pointer is valid, execute the onClick function */
    if(btn->button->pressed && btn->onClick.fn != NULL)
        oe_executeOnEventfn(btn->onClick);
}

void btn_drawButton(btn_Button *btn, SDL_Renderer *renderer, bool displayHitboxOutline)
{
    /* Draw the sprite */
    spr_drawSprite(btn->sprite, renderer);

    /* Draw the hitbox outline */
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
