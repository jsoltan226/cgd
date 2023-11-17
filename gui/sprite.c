#include "sprite.h"
#include "../util.h"
#include <SDL2/SDL_render.h>
#include <assert.h>

spr_Sprite* spr_initSprite(spr_SpriteConfig* cfg, SDL_Renderer* renderer)
{
    spr_Sprite* spr = malloc(sizeof(spr_Sprite));
    assert(spr != NULL);

    spr->srcRect = cfg->srcRect;
    spr->destRect = cfg->destRect;
    spr->hitbox = cfg->hitbox;
    spr->texture = u_loadPNG(renderer, cfg->textureFilePath);

    return spr;
}

void spr_drawSprite(spr_Sprite* s, SDL_Renderer* renderer)
{
    SDL_RenderCopy(renderer, s->texture, &s->srcRect, &s->destRect);
}

void spr_destroySprite(spr_Sprite* spr)
{
    SDL_DestroyTexture(spr->texture);

    free(spr);
    spr = NULL;
}
