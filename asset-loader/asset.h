#ifndef ASSET_H_
#define ASSET_H_

#include <SDL2/SDL_render.h>
#include "core/int.h"
#include "core/pixel.h"
#include "img-type.h"

#define ASSET_MAX_FILEPATH_LEN 256
typedef const char filepath_t[ASSET_MAX_FILEPATH_LEN];

struct asset {
    filepath_t rel_file_path;
    enum asset_img_type type;

    struct pixel_flat_data pixel_data;
    SDL_Texture *texture;
};

/* Both return NULL on failure */
struct asset * asset_load(filepath_t rel_file_path, SDL_Renderer *renderer);
FILE * asset_fopen(const char *rel_file_path, const char *mode);

/* If `rel_file_path` or `img_type` are NULL,
 * then the function will use the ones from the given asset struct */
/* Returns 0 on success and a non-zero error code on failure */
i32 asset_write(struct asset *asset, const char *rel_file_path, enum asset_img_type img_type);

void asset_destroy(struct asset *asset);

/* Get the absolute path to the directory in which assets are stored */
const char * asset_get_assets_dir();

#endif /* ASSET_H_ */
