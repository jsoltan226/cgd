#include "util.h"
#include <png.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_surface.h>

SDL_Texture* u_loadPNG(SDL_Renderer* renderer, const char* restrict filePath)
{
    /* (Try to) Open the given file */
    FILE *file = fopen(filePath, "rb");
    if (!file) {
        fprintf(stderr, "[u_loadPNG] Error: File '%s' could not be opened for reading.\n", filePath);
        return NULL;
    }

    /* Read PNG header */
    png_byte header[8];
    fread(header, 1, 8, file);
    if (png_sig_cmp(header, 0, 8)) {
        fprintf(stderr, "[u_loadPNG] Error: %s is not a valid PNG file.\n", filePath);
        fclose(file);
        return NULL;
    }

    /* Initialize libpng structures */
    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png) {
        fclose(file);
        return NULL;
    }

    png_infop info = png_create_info_struct(png);
    if (!info) {
        png_destroy_read_struct(&png, NULL, NULL);
        fclose(file);
        return NULL;
    }

    if (setjmp(png_jmpbuf(png))) {
        png_destroy_read_struct(&png, &info, NULL);
        fclose(file);
        return NULL;
    }

    /* Set up libpng to read from file */
    png_init_io(png, file);
    png_set_sig_bytes(png, 8);

    /* Read PNG information */
    png_read_info(png, info);

    int width = png_get_image_width(png, info);
    int height = png_get_image_height(png, info);
    png_byte colorType = png_get_color_type(png, info);
    png_byte bitDepth = png_get_bit_depth(png, info);

    /* Set transformations if needed (e.g., expand palette images to RGB) */
    if (colorType == PNG_COLOR_TYPE_PALETTE) {
        png_set_palette_to_rgb(png);
    }

    if (bitDepth == 16) {
        png_set_strip_16(png);
    }

    if (png_get_valid(png, info, PNG_INFO_tRNS)) {
        png_set_tRNS_to_alpha(png);
    }

    if (colorType == PNG_COLOR_TYPE_GRAY || colorType == PNG_COLOR_TYPE_GRAY_ALPHA) {
        png_set_gray_to_rgb(png);
    }

    /* Update PNG information after transformations */
    png_read_update_info(png, info);

    /* Allocate memory for image data */
    png_bytep *rowPointers = (png_bytep *)malloc(sizeof(png_bytep) * height);
    for (int y = 0; y < height; y++) {
        rowPointers[y] = (png_byte *)malloc(png_get_rowbytes(png, info));
    }

    /* Read PNG image data */
    png_read_image(png, rowPointers);

    /* Create SDL_Surface and copy image data */
    SDL_Surface *tmpSurface = SDL_CreateRGBSurfaceWithFormat(0, width, height, 32, SDL_PIXELFORMAT_RGBA32);
    if (!tmpSurface) {
        fclose(file);
        return NULL;
    }

    for (int y = 0; y < height; y++) {
        memcpy(tmpSurface->pixels + y * tmpSurface->pitch, rowPointers[y], tmpSurface->pitch);
        /* The value in rowPointers[y] will no longer be needed, so free it */
        free(rowPointers[y]);
    }

    /* We are done with libpng, so free all the unneeded png structures */
    free(rowPointers);
    fclose(file);
    png_destroy_read_struct(&png, &info, NULL); 

    /* Finally, create a texture from the temporary surface and return it */
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, tmpSurface);
    SDL_FreeSurface(tmpSurface);
    return texture;
}

/* You already know what this does */
int u_max(int a, int b)
{
    if(a > b)
        return a;
    else 
        return b;
}

/* Same here */
int u_min(int a, int b)
{
    if(a < b)
        return a;
    else 
        return b;
}

/* The simplest collision checking implementation;
 * returns true if 2 rectangles overlap 
 */
bool u_collision(const SDL_Rect *r1, const SDL_Rect *r2)
{
    return (
            r1->x + r1->w >= r2->x &&
            r1->x <= r2->x + r2->w &&
            r1->y + r1->h >= r2->y &&
            r1->y <= r2->y + r2->h 
           );
}
