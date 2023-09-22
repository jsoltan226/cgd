#ifndef UTIL_H
#define UTIL_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdbool.h>

SDL_Texture* loadImage(SDL_Renderer* renderer, const char* filePath);
int max(int a, int b);
bool collision(const SDL_Rect r1, const SDL_Rect r2);

#endif
