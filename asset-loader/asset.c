#include "asset.h"
#include "img-type.h"
#include "io-PNG.h"
#include "raw2sdl.h"
#include "util/pixel.h"
#include <SDL2/SDL_render.h>
#include <stdio.h>
#include <stdlib.h>
#include <cgd/util/util.h>
#include <string.h>

struct asset * asset_load(const char *rel_file_path, SDL_Renderer *r)
{
    FILE *fp = NULL;

    struct asset *a = malloc(sizeof(struct asset));
    if (a == NULL) return NULL;

    a->pixel_data = malloc(sizeof(struct pixel_data));
    if (a->pixel_data == NULL)
        goto err;

    a->rel_file_path = rel_file_path;
    fp = asset_fopen(rel_file_path, "rb");
    if (fp == NULL)
        goto err;

    a->type = asset_get_img_type(fp);
    if (a->type == IMG_TYPE_UNKNOWN)
        goto err;
    
    switch(a->type) {
        case IMG_TYPE_PNG:
            if (read_PNG(a->pixel_data, fp))
                goto err;
            break;
        case IMG_TYPE_UNKNOWN: default:
            goto err;
            break;
    }

    a->texture = pixel_data_2_sdl_tex(a->pixel_data, r);
    if (a->texture == NULL) goto err;

    fclose(fp);
    return a;

err:
    if (fp) fclose(fp);
    if (a) asset_destroy(a);
    return NULL;
}

FILE * asset_fopen(const char *rel_file_path, const char *mode)
{
    FILE *fp = NULL;
    char full_path_buf[u_BUF_SIZE] = { 0 };

    strncpy(full_path_buf, u_get_asset_dir(), u_BUF_SIZE - 1);
    strncat(full_path_buf, rel_file_path, u_BUF_SIZE - strlen(full_path_buf) - 1);

    fp = fopen(full_path_buf, mode);
    return fp; /* NULL will be returned if fp is NULL */
}

i32 asset_write(struct asset *a, const char *rel_file_path, enum asset_img_type img_type)
{
    if (a == NULL) return -1;

    if (rel_file_path == NULL) 
        rel_file_path = a->rel_file_path;
    if (img_type == IMG_TYPE_UNKNOWN)
        img_type = a->type;

    FILE *fp = asset_fopen(rel_file_path, "wb");

    switch(a->type) {
        case IMG_TYPE_PNG:
            if (write_PNG(a->pixel_data, fp))
                return -2;
        case IMG_TYPE_UNKNOWN: default:
            return -3;
    }

    return EXIT_SUCCESS;
}

void asset_destroy(struct asset *a)
{
    if (a == NULL) return;

    if (a->pixel_data->data) 
        destroy_pixel_data(a->pixel_data);

    if (a->texture)
        SDL_DestroyTexture(a->texture);
}
