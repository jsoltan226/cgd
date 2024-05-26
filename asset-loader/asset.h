#ifndef ASSET_H_
#define ASSET_H_

#include <SDL2/SDL_render.h>
#include <cgd/util/int.h>
#include <cgd/util/pixel.h>
#include "img-type.h"

struct asset {
    const char *rel_file_path;
    enum asset_img_type type;
    
    struct pixel_data *pixel_data;
    SDL_Texture *texture;
};

/* Both return NULL on failure */
struct asset * asset_load(const char *rel_file_path, SDL_Renderer *renderer);
FILE * asset_fopen(const char *rel_file_path, const char *mode);

/* If `rel_file_path` or `img_type` are NULL,
 * then the function will use the ones from the given asset struct */
/* Returns 0 on success and a non-zero error code on failure */
i32 asset_write(struct asset *asset, const char *rel_file_path, enum asset_img_type img_type);

void asset_destroy(struct asset *asset);

#endif /* ASSET_H_ */
