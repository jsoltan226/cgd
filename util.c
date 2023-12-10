#include "util.h"
#include <png.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_surface.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

const char *u_getAssetPath()
{
    if (_cgd_util_internal_binDirBuffer[0] != '/') {
        if (u_getBinDir(_cgd_util_internal_binDirBuffer, u_BUF_SIZE)) return NULL;
    }
    if (_cgd_util_internal_assetsDirBuffer[0] != '/') {
        if (strncpy(_cgd_util_internal_assetsDirBuffer, _cgd_util_internal_binDirBuffer, u_BUF_SIZE - 1) == NULL) return NULL;
        if (strncat(_cgd_util_internal_assetsDirBuffer, u_PATH_FROM_BIN_TO_ASSETS, u_BUF_SIZE - strlen(_cgd_util_internal_assetsDirBuffer) - 1) == NULL) return NULL;
    }

    return _cgd_util_internal_assetsDirBuffer;
}

SDL_Texture* u_loadPNG(const char* filePath, SDL_Renderer* renderer)
{
    /* Get the real path to the asset */
    char realPath[u_BUF_SIZE] = { 0 };
    if (strncpy(realPath, u_getAssetPath(), u_BUF_SIZE - 1) == NULL ||
        strncat(realPath, filePath, u_BUF_SIZE - strlen(realPath) - 1) == NULL )
    {
        fprintf(stderr, "[u_loadPNG] Error: Failed to get the real path of the given (%s) asset.\n", filePath);
        return NULL;
    }


    /* (Try to) Open the given file */
    FILE *file = fopen(realPath, "rb");
    if (!file) {
        fprintf(stderr, "[u_loadPNG] Error: File '%s' could not be opened for reading.\n", realPath);
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

void u_error(const char *fmt, ...)
{
    va_list vaList;
    va_start(vaList, fmt);
    vfprintf(stderr, fmt, vaList);
    va_end(vaList);
}

int u_getBinDir(char *buf, size_t buf_size)
{
    if (buf == NULL) {
        fprintf(stderr, "[getBinDir]: ERROR: The provided string is a null pointer.\n");
        return EXIT_FAILURE;
    }
    memset(buf, 0, buf_size);

    char fullPathBuf[buf_size];
    fullPathBuf[buf_size - 1] = '\0';

    if (!readlink("/proc/self/exe", fullPathBuf, buf_size - 1)) {
        fprintf(stderr, "[getBinDir]: ERROR: Readlink for /proc/self/exe failed.\n");
        return EXIT_FAILURE;
    }

    int i = 0;
    int binDirLen  = 0;
    do {
        if(fullPathBuf[i] == '/') binDirLen = i + 1; /* We want the trailing '/' */
    } while (fullPathBuf[i++]);

    if (strncpy(buf, fullPathBuf, binDirLen) == NULL) {
        fprintf(stderr, "[getBinDir]: ERROR: Failed to copy the bin dir path into the provided buffer.\n");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
