#include "sprite.h"
#include <cgd/asset-loader/images.h>
#include <SDL2/SDL_render.h>
#include <assert.h>

spr_Sprite* spr_initSprite(spr_SpriteConfig* cfg, SDL_Renderer* renderer)
{
    /* Allocate the sprite struct on the heap */
    spr_Sprite* spr = malloc(sizeof(spr_Sprite));
    assert(spr != NULL);

    /* Copy over the info given in the configuration */
    spr->srcRect = cfg->srcRect;
    spr->destRect = cfg->destRect;
    spr->hitbox = cfg->hitbox;

    /* Load the texture from an image stored in cfg->textureFilePath */
    spr->texture = u_loadPNG(cfg->textureFilePath, renderer);
    assert(spr->texture != NULL);

    return spr;
}

void spr_drawSprite(spr_Sprite* s, SDL_Renderer* renderer)
{
    /* Render the sprite */
    SDL_RenderCopy(renderer, s->texture, &s->srcRect, &s->destRect);
}

void spr_destroySprite(spr_Sprite* spr)
{
    /* for god's sake, you can guess what this function does */
    SDL_DestroyTexture(spr->texture);

    free(spr);
    spr = NULL;
}
