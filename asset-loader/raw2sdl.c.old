#include "raw2sdl.h"
#include <core/log.h>
#include <core/util.h>
#include <core/pixel.h>
#include <string.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_error.h>
#include <SDL2/SDL_render.h>

#define MODULE_NAME "raw2sdl"

#define N_CHANNELS  4

SDL_Texture * pixel_data_2_sdl_tex(struct pixel_flat_data *pixel_data, SDL_Renderer *renderer)
{
    u_check_params(pixel_data != NULL && renderer != NULL);
    SDL_Surface *tmpSurface = NULL;

    /* Copy over the pixels to an SDL_Surface */
    /* We use 32 as the pixel format, becasuse each of our pixels has 4 channels 8 bits each, so 4 * 8 = 32 */
    tmpSurface = SDL_CreateRGBSurfaceWithFormat(0, pixel_data->w, pixel_data->h, sizeof(pixel_t), SDL_PIXELFORMAT_RGBA32);
    if (tmpSurface == NULL) {
        s_log_error("Failed to create SDL_Surface: %s", SDL_GetError());
        return NULL;
    }

    /* Copy over the data */
    SDL_LockSurface(tmpSurface);
    memcpy(tmpSurface->pixels, pixel_data->buf, pixel_data->w * pixel_data->h * sizeof(pixel_t));
    SDL_UnlockSurface(tmpSurface);

    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, tmpSurface);
    if (texture == NULL)
        s_log_error("Failed to create an SDL_Texture from tmpSurface: %s", SDL_GetError());

    SDL_FreeSurface(tmpSurface);

    return texture;
}
