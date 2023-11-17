#ifndef SPRITE_H
#define SPRITE_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_render.h>

typedef struct {
    SDL_Rect hitbox;
    SDL_Rect srcRect, destRect;
    SDL_Texture* texture;
} spr_Sprite;

typedef struct {
    SDL_Rect hitbox, srcRect, destRect;
    const char* textureFilePath;
} spr_SpriteConfig;

spr_Sprite* spr_initSprite(spr_SpriteConfig* cfg, SDL_Renderer* r);
void spr_drawSprite(spr_Sprite* spr, SDL_Renderer* r);
void spr_destroySprite(spr_Sprite* spr);

#endif
