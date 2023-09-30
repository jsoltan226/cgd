#ifndef UTIL_H
#define UTIL_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdbool.h>

typedef struct {
    SDL_Rect srcRect, destRect, hitbox;
    SDL_Texture* texture;
} u_Sprite;

void u_initSprite(u_Sprite* s, SDL_Rect srcRect, SDL_Rect destRect, SDL_Rect hitbox, const char* textureFilePath, SDL_Renderer* r);
void u_drawSprite(const u_Sprite* sprite, SDL_Renderer* renderer);

SDL_Texture* u_loadImage(SDL_Renderer* renderer, const char* filePath);
int u_max(int a, int b);
bool u_collision(const SDL_Rect r1, const SDL_Rect r2);

#endif
