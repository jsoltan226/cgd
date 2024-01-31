#ifndef UTIL_IMAGES_H
#define UTIL_IMAGES_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_render.h>

SDL_Texture* u_loadPNG(const char* filePath, SDL_Renderer* renderer);
#endif /* UTIL_IMAGES_H */
