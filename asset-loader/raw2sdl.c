#include "raw2sdl.h"
#include "core/pixel.h"
#include "core/int.h"
#include "core/log.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_error.h>
#include <SDL2/SDL_render.h>
#include <string.h>

#define N_CHANNELS  4

SDL_Texture * pixel_data_2_sdl_tex(struct pixel_data *pixel_data, SDL_Renderer *renderer)
{
    if (pixel_data == NULL || renderer == NULL) {
        s_log_error("raw2sdl", "Invalid parameters");
        return NULL;
    }
    SDL_Surface *tmpSurface = NULL;

    /* Copy over the pixels to an SDL_Surface */
    /* We use 32 as the pixel format, becasuse each of our pixels has 4 channels 8 bits each, so 4 * 8 = 32 */
    tmpSurface = SDL_CreateRGBSurfaceWithFormat(0, pixel_data->w, pixel_data->h, sizeof(pixel_t), SDL_PIXELFORMAT_RGBA32);
    if (tmpSurface == NULL) {
        s_log_error("raw2sdl", "Failed to create SDL_Surface: %s", SDL_GetError());
        return NULL;
    }

    /* Copy over the data */
    SDL_LockSurface(tmpSurface);
    for (u32 y = 0; y < pixel_data->h; y++) {
        memcpy(tmpSurface->pixels + y*tmpSurface->pitch, pixel_data->data[y], tmpSurface->pitch);
    }
    SDL_UnlockSurface(tmpSurface);

    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, tmpSurface);
    if (texture == NULL)
        s_log_error("raw2sdl", "Failed to create an SDL_Texture from tmpSurface: %s", SDL_GetError());

    SDL_FreeSurface(tmpSurface);

    return texture;
}
