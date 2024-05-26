#ifndef SPRITE_H
#define SPRITE_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_render.h>
#include <cgd/util/shapes.h>
#include <cgd/asset-loader/asset.h>

typedef struct {
    rect_t hitbox;
    rect_t srcRect, destRect;
    struct asset *asset;
} spr_Sprite;

typedef struct {
    rect_t hitbox, srcRect, destRect;
    const char* textureFilePath;
} spr_SpriteConfig;

spr_Sprite* spr_initSprite(spr_SpriteConfig* cfg, SDL_Renderer* r);
void spr_drawSprite(spr_Sprite* spr, SDL_Renderer* r);
void spr_destroySprite(spr_Sprite* spr);

#endif
