#include "raw2sdl.h"
#include <cgd/util/pixel.h>
#include <cgd/util/int.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_render.h>
#include <string.h>

#define N_CHANNELS  4
SDL_Texture * pixel_data_2_sdl_tex(struct pixel_data *pixel_data, SDL_Renderer *renderer)
{
    SDL_Surface *tmpSurface = NULL;

    /* Copy over the pixels to an SDL_Surface */
    /* We use 32 as the pixel format, becasuse each of our pixels has 4 channels 8 bits each, so 4 * 8 = 32 */
    tmpSurface = SDL_CreateRGBSurfaceWithFormat(0, pixel_data->w, pixel_data->h, sizeof(pixel_t), SDL_PIXELFORMAT_RGBA32);
    if (tmpSurface == NULL) return NULL;

    /* Copy over the data */
    SDL_LockSurface(tmpSurface);
    for (u32 y = 0; y < pixel_data->h; y++) {
        memcpy(tmpSurface->pixels + y*tmpSurface->pitch, pixel_data->data[y], tmpSurface->pitch);
        /*
        for (u32 x = 0; x < pixel_data->h; x++) {
            u8 *pixel_p = (u8*)tmpSurface->pixels + y*tmpSurface->pitch + x*N_CHANNELS;
            for (int i = 0; i < N_CHANNELS; i++) {
                *(pixel_p + i) = pixel_data->data[y][x*N_CHANNELS + i];
            }

        }
        */
    }
    SDL_UnlockSurface(tmpSurface);

    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, tmpSurface);
    /* NULL will be returned when the texture is NULL */

    SDL_FreeSurface(tmpSurface);

    return texture;
}