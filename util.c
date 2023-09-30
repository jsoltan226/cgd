#include "util.h"
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_surface.h>

SDL_Texture* u_loadImage(SDL_Renderer* renderer, const char* restrict filePath)
{
    SDL_Surface* tmpSurface = IMG_Load(filePath);
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, tmpSurface);
    SDL_FreeSurface(tmpSurface);
    return texture;
}

int u_max(int a, int b)
{
    if(a > b)
        return a;
    else 
        return b;
}

bool u_collision(const SDL_Rect r1, const SDL_Rect r2)
{
    return (
            r1.x + r1.w >= r2.x &&
            r1.x <= r2.x + r2.w &&
            r1.y + r1.h >= r2.y &&
            r1.y <= r2.y + r2.h 
           );
}
