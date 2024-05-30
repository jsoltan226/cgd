#include "asset.h"
#include "img-type.h"
#include "io-PNG.h"
#include "raw2sdl.h"
#include "util/pixel.h"
#include <SDL2/SDL_render.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <cgd/util/util.h>
#include <cgd/util/log.h>
#include <string.h>

#define module_name

#define goto_error(msg...) do {         \
    s_log_error(module_name, msg);     \
    goto err;                           \
} while (0);
#undef module_name

struct asset * asset_load(const char *rel_file_path, SDL_Renderer *r)
{
#define module_name "asset-load"
    s_log_debug("asset-load", "Loading asset \"%s\"...", rel_file_path);

    FILE *fp = NULL;

    struct asset *a = calloc(1, sizeof(struct asset));
    if (a == NULL)
        goto_error("Failed to malloc() asset struct!");

    a->pixel_data = calloc(1, sizeof(struct pixel_data));
    if (a->pixel_data == NULL)
        goto_error("Failed to malloc() asset pixel data!");

    a->rel_file_path = rel_file_path;
    fp = asset_fopen(rel_file_path, "rb");
    if (fp == NULL)
        goto_error("Failed to open asset \"%s\": %s", a->rel_file_path, strerror(errno));

    a->type = asset_get_img_type(fp);
    if (a->type == IMG_TYPE_UNKNOWN)
        goto_error("Asset image type is UNKNOWN!");
    
    switch(a->type) {
        case IMG_TYPE_PNG:
            if (read_PNG(a->pixel_data, fp))
                goto_error("read_PNG Failed for \"%s\"", a->rel_file_path);
            break;
        case IMG_TYPE_UNKNOWN: default:
            goto err;
            break;
    }

    a->texture = pixel_data_2_sdl_tex(a->pixel_data, r);
    if (a->texture == NULL)
        goto_error("Failed to create an SDL_Texture from the pixel_data!");

    fclose(fp);
    return a;

err:
    if (fp) fclose(fp);
    if (a) asset_destroy(a);
    return NULL;
#undef module_name
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
    if (a == NULL) {
        s_log_error("asset-write", "Arg `struct asset *a` is NULL!");
        return -1;
    }

    if (rel_file_path == NULL) 
        rel_file_path = a->rel_file_path;
    if (img_type == IMG_TYPE_UNKNOWN)
        img_type = a->type;

    s_log_debug("asset-write", "Writing asset \"%s\" to path \"%s\"", a->rel_file_path, rel_file_path);

    FILE *fp = asset_fopen(rel_file_path, "wb");
    if (fp == NULL) {
        s_log_error("asset-write", "Failed to open out file \"%s\": %s", rel_file_path, strerror(errno));
        return -2;
    }

    switch(a->type) {
        case IMG_TYPE_PNG:
            if (write_PNG(a->pixel_data, fp)) {
                s_log_error("asset-write", "write_PNG failed for \"%s\"", a->rel_file_path);
                fclose(fp);
                return -3;
            }
        case IMG_TYPE_UNKNOWN: default:
            s_log_error("asset-write", "image type of \"%s\" is UNKNOWN!", a->rel_file_path);
            fclose(fp);
            return -4;
    }

    fclose(fp);

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
