#include "gutils.h"
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_surface.h>

SDL_Texture* loadImage(SDL_Renderer* renderer, const char* restrict filePath)
{
    SDL_Surface* tmpSurface = IMG_Load(filePath);
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, tmpSurface);
    SDL_FreeSurface(tmpSurface);
    return texture;
}

int max(int a, int b)
{
    if(a > b)
        return a;
    else 
        return b;
}
