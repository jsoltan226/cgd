#ifndef RAW2SDL_H_
#define RAW2SDL_H_

#include <SDL2/SDL.h>
#include <SDL2/SDL_render.h>
#include "core/pixel.h"

/* Returns NULL on failure */
SDL_Texture * pixel_data_2_sdl_tex(struct pixel_flat_data *pixel_data, SDL_Renderer *renderer);

#endif /* RAW2SDL_H_ */
