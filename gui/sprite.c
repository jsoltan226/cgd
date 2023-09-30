#include "sprite.h"
#include "../util.h"
#include <SDL2/SDL_render.h>

void spr_createSprite(spr_Sprite* s, SDL_Renderer* renderer, SDL_Rect hitbox, SDL_Rect srcRect, SDL_Rect destRect, const char* textureFilePath)
{
    s->srcRect = srcRect;
    s->destRect = destRect;
    s->hitbox = hitbox;
    s->texture = NULL;
}

void spr_initSprite(spr_Sprite* s, SDL_Renderer* renderer, const char* textureFilePath)
{
    s->texture = u_loadImage(renderer, textureFilePath);
}

void spr_drawSprite(spr_Sprite s, SDL_Renderer* renderer)
{
    SDL_RenderCopy(renderer, s.texture, &s.srcRect, &s.destRect);
}

void spr_destroySprite(spr_Sprite* s)
{
    SDL_DestroyTexture(s->texture);
}
