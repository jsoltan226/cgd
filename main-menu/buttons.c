#include "buttons.h"
#include "../util.h"
#include <SDL2/SDL_render.h>
#include <stdio.h>
#include <stdlib.h>

int btn_onClick_printMessage(){ printf("Button pressed!\n"); return EXIT_SUCCESS; }

void btn_initButton(
        btn_Button* button, SDL_Renderer* renderer, 
        int (*onClick) (),
        const SDL_Rect hitbox, const SDL_Rect srcRect, const SDL_Rect destRect, const char* textureFilePath)
{
    button->hitbox = hitbox;
    button->srcRect = srcRect;
    button->destRect = destRect;
    button->onClick = onClick;
    button->pressed = false;
    button->time = 0;
    button->texture = loadImage(renderer, textureFilePath);
}

void btn_updateButton(btn_Button* button, input_Mouse mouse)
{
    const SDL_Rect mouseRect = { mouse.x, mouse.y, 0, 0 };
    bool hovering = collision(mouseRect, button->hitbox);

    button->pressed = false;
    if(hovering && mouse.buttonLeft.pressed)
        button->pressed = true;
    if(hovering && mouse.buttonLeft.down)
        if(button->onClick() == EXIT_FAILURE)
            fprintf(stderr, "Error: Button onClick function failed to execute!\n");
}

void btn_drawButton(btn_Button button, SDL_Renderer* renderer, bool displayHitboxOutline)
{
    if(displayHitboxOutline){
        SDL_Color outlineColor;
        if(button.pressed)
            outlineColor = btn_hitboxOutlineColorPressed;
        else
            outlineColor = btn_hitboxOutlineColorNormal;

        SDL_SetRenderDrawColor(renderer, outlineColor.r, outlineColor.g, outlineColor.b, outlineColor.a);
    }

    SDL_RenderCopy(renderer, button.texture, &button.srcRect, &button.destRect);
    SDL_RenderDrawRect(renderer, &button.hitbox );
}

void btn_destroyButton(btn_Button* button)
{
    SDL_DestroyTexture(button->texture);
}
