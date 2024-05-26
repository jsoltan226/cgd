#include "sprite.h"
#include <cgd/asset-loader/asset.h>
#include <SDL2/SDL_render.h>
#include <assert.h>

spr_Sprite* spr_initSprite(spr_SpriteConfig* cfg, SDL_Renderer* renderer)
{
    /* Allocate the sprite struct on the heap */
    spr_Sprite* spr = malloc(sizeof(spr_Sprite));
    assert(spr != NULL);

    spr->srcRect = cfg->srcRect;
    spr->destRect = cfg->destRect;
    spr->hitbox = cfg->hitbox;

    spr->asset = asset_load(cfg->textureFilePath, renderer);
    assert(spr->asset != NULL);

    return spr;
}

void spr_drawSprite(spr_Sprite* s, SDL_Renderer* renderer)
{
    SDL_RenderCopy(renderer,
        s->asset->texture,
        (const SDL_Rect *)&s->srcRect,
        (const SDL_Rect *)&s->destRect
    );
}

void spr_destroySprite(spr_Sprite* spr)
{
    asset_destroy(spr->asset);
    free(spr);
}
