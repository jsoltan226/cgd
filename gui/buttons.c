#include "buttons.h"
#include "../util.h"
#include "sprite.h"
#include <SDL2/SDL_render.h>
#include <stdio.h>
#include <stdlib.h>

void btn_initButton(btn_Button* b, spr_Sprite sprite, SDL_Renderer* renderer, const char* textureFilePath)
{
    printf("made it to initButton function with texture file path: %s\n", textureFilePath);
    b->sprite = sprite;
    po_initPressableObj(&b->button);
    spr_initSprite(&b->sprite, renderer, textureFilePath);
}

void btn_updateButton(btn_Button* b, input_Mouse mouse)
{
    const SDL_Rect mouseRect = { mouse.x, mouse.y, 0, 0 };
    bool hovering = u_collision(mouseRect, b->sprite.hitbox);

    po_updatePressableObj(&b->button, hovering && mouse.buttonLeft.pressed);
}

void btn_drawButton(btn_Button b, SDL_Renderer* renderer, bool displayHitboxOutline)
{
    spr_drawSprite(b.sprite, renderer);

    if(displayHitboxOutline){
        SDL_Color outlineColor;
        if(b.button.pressed)
            outlineColor = btn_hitboxOutlineColorPressed;
        else
            outlineColor = btn_hitboxOutlineColorNormal;

        SDL_SetRenderDrawColor(renderer, outlineColor.r, outlineColor.g, outlineColor.b, outlineColor.a);
    }
    SDL_RenderDrawRect(renderer, &b.sprite.hitbox);
}

void btn_destroyButton(btn_Button* button)
{
    spr_destroySprite(&button->sprite);
}
