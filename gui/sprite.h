#ifndef SPRITE_H
#define SPRITE_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_render.h>

typedef struct {
    SDL_Rect hitbox;
    SDL_Rect srcRect, destRect;
    SDL_Texture* texture;
} spr_Sprite;

void spr_createSprite(spr_Sprite* s, SDL_Renderer* r, SDL_Rect hitbox, SDL_Rect srcRect, SDL_Rect destRect, const char* textureFilePath);
void spr_initSprite(spr_Sprite* s, SDL_Renderer* r, const char* textureFilePath);
void spr_drawSprite(spr_Sprite s, SDL_Renderer* r);
void spr_destroySprite(spr_Sprite* s);

#endif
