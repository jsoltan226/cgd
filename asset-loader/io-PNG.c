#include "io-PNG.h"
#include "core/log.h"
#include "core/int.h"
#include "core/pixel.h"
#include <err.h>
#include <png.h>
#include <stdlib.h>
#include <string.h>

/* static void printPngInfo(png_structp png_ptr, png_infop info_ptr); */

/* This struct groups all the input image metadata, such as width, height,
 * compression method, etc.
 * PNG supports various methods of storing the image that we're not used to,
 * and so any program using libPNG has to account for them.
 */
struct PNG_Metadata {
    png_byte header[PNG_MAGIC_SIZE];

    /* Image header metadata (IHDR) */
    u32 width, height;
    i32 bitDepth, colorType, interlaceMethod, compressionMethod, filterMethod;

    /* Color Palette (PLTE) - Custom color palette */
    png_colorp palette;
    i32 n_palette;

    /* An alternative way to store transparency AKA alpha channel (tRNS) */
    png_bytep trans_alpha;
    i32 n_trans;
    png_color_16p trans_color;

    /* Image gamma (gAMA) */
    f64 gamma;
};

enum load_PNG_error_code {
    ERR_INPUT_FILEP_NULL,
    ERR_NOT_PNG,
    ERR_INIT_PNG_STRUCT,
    ERR_INIT_PNG_READ_INFO,
    ERR_INIT_PNG_END,
    ERR_SETJMP,
    ERR_ALLOC_PIXELDATA,
} load_PNG_error_code;

#define goto_error(code, msg...) do { \
    load_PNG_error_code = code; \
    s_log_error("io-PNG", msg); \
    goto err_cleanup_l; \
} while (0);

#define N_PNG_SIG_BYTES 8
#define N_CHANNELS 4
i32 read_PNG(struct pixel_data *pixel_data, FILE *fp)
{
    png_structp png_ptr = NULL;
    png_infop info_ptr = NULL;
    png_infop end_ptr = NULL;

    struct PNG_Metadata meta;

    if (fp == NULL)
        goto_error(ERR_INPUT_FILEP_NULL, "The in file pointer is NULL");

    /* Read the PNG header to check whether the given file contains a valid PNG signature */
    fread(meta.header, 1, N_PNG_SIG_BYTES, fp);
    if (png_sig_cmp(meta.header, 0, N_PNG_SIG_BYTES))
        goto_error(ERR_NOT_PNG, "The file is not a valid PNG (magic bytes missing)!");

    /** Initialization and setup of libpng structures **/
    /* Initialize png struct */
    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
        NULL, NULL, NULL
    );
    if (png_ptr == NULL)
        goto_error(ERR_INIT_PNG_STRUCT, "Failed to create the read PNG struct");

    /* Initialize png info struct */
    info_ptr = png_create_info_struct(png_ptr);
    if (info_ptr == NULL)
        goto_error(ERR_INIT_PNG_READ_INFO, "Failed to create the read PNG info struct");

    /* Not creating the end_ptr sometimes caused a weird memory error I don't understand */
    end_ptr = png_create_info_struct(png_ptr);
    if (end_ptr == NULL)
        goto_error(ERR_INIT_PNG_END, "Failed to create the read PNG end info struct");

    /* Setting up some (as usual) weird and non-standard error handling */
    /* From the libpng manual:
     * "When libpng encounters an error, it expects to longjmp back
to your routine.  Therefore, you will need to call setjmp and pass
your png_jmpbuf(png_ptr)."
    */
    if (setjmp(png_jmpbuf(png_ptr)))
        goto_error(ERR_SETJMP, "setjmp() failed when trying to set up PNG error handling");

    /* Set up the PNG input code */
    png_init_io(png_ptr, fp);

    /* Inform libpng that we have read the signature bytes */
    png_set_sig_bytes(png_ptr, N_PNG_SIG_BYTES);

    /** Read image metadata **/
    png_read_info(png_ptr, info_ptr);
    png_get_IHDR(png_ptr, info_ptr, &meta.width, &meta.height, &meta.bitDepth, &meta.colorType,
            &meta.interlaceMethod, &meta.compressionMethod, &meta.filterMethod);

    png_get_PLTE(png_ptr, info_ptr, &meta.palette, &meta.n_palette);

    png_get_tRNS(png_ptr, info_ptr, &meta.trans_alpha, &meta.n_trans, &meta.trans_color);

    png_get_gAMA(png_ptr, info_ptr, &meta.gamma);

    /** Make sure the raw image data is transformed
     ** to standard RGBA8888 (TrueColor) format
     **/
    /* And yes, any other self-respecting library would do that automatically with a sigle function call... */

    /* Make sure each color channel is 8 bits (as it could be 16) */
    if (meta.bitDepth == 16)
        png_set_strip_16(png_ptr);

    /* PLTE (Pallette) is an alternative method of storing color values.
     * We need to make sure to transform it to our standard RGBA format */
    if (meta.colorType == PNG_COLOR_TYPE_PALETTE)
        png_set_palette_to_rgb(png_ptr);

    /* tRNS (Transparency) is an alternative way of storing the alpha channel.
     * We need to transform this to fit the RGBA format */
    if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
        png_set_tRNS_to_alpha(png_ptr);

    /* I have no idea what this does */
    if (meta.colorType == PNG_COLOR_TYPE_RGB ||
        meta.colorType == PNG_COLOR_TYPE_GRAY ||
        meta.colorType == PNG_COLOR_TYPE_PALETTE)
    {
        png_set_filler(png_ptr, 0xFF, PNG_FILLER_AFTER);
    }

    /* PNG supports grayscale images
     * (Just one gray (G) channel instead of three for RGB).
     * We obviously need to transofrm this to RGB
     */
    if (meta.colorType == PNG_COLOR_TYPE_GRAY || meta.colorType == PNG_COLOR_TYPE_GRAY_ALPHA)
        png_set_gray_to_rgb(png_ptr);

    /* Again, make surte that the amount of bits per channel is 8 */
    if (meta.colorType == PNG_COLOR_TYPE_GRAY && meta.bitDepth < 8)
        png_set_expand_gray_1_2_4_to_8(png_ptr);

    /* Apply all transformations needed to get the raw data in RGBA8888 */
    png_read_update_info(png_ptr, info_ptr);

    /* Allocate memory for the raw pixel data */
    pixel_data->data = malloc(meta.height * sizeof(u8*));
    if(pixel_data == NULL)
        goto_error(ERR_ALLOC_PIXELDATA, "Failed to malloc() pixel_data->data");

    for (u32 i = 0; i < meta.height; i++) {
        pixel_data->data[i] = malloc(meta.width * N_CHANNELS);
        if (pixel_data->data[i] == NULL)
            goto_error(ERR_ALLOC_PIXELDATA, "Failed to malloc() pixel_data->data[%u]", i);
    }

    /* And now, read the pixel data!!! FINALLY!!! */
    png_read_image(png_ptr, pixel_data->data);
    pixel_data->w = meta.width;
    pixel_data->h = meta.height;

    /* Now we can get rid of the input file pointer and PNG structs */
    png_destroy_read_struct(&png_ptr, &info_ptr, &end_ptr);

    return EXIT_SUCCESS;

err_cleanup_l:
    if (end_ptr) png_destroy_info_struct(png_ptr, &end_ptr);
    if (info_ptr) png_destroy_info_struct(png_ptr, &info_ptr);
    if (png_ptr) png_destroy_read_struct(&png_ptr, NULL, NULL);
    return load_PNG_error_code;
}
#undef goto_error

enum write_PNG_error_code {
    ERR_OUTPUT_FILEP_NULL,
    ERR_INVALID_PIXELDATA,
    ERR_INVALID_WIDTH,
    ERR_INVALID_HEIGHT,
    ERR_INIT_PNG_WRITE_STRUCT,
    ERR_INIT_PNG_WRITE_INFO,
} write_PNG_error_code;
#define goto_error(code, msg...) do { \
    write_PNG_error_code = code; \
    s_log_error("io-PNG", msg); \
    goto err_l; \
} while (0);

i32 write_PNG(struct pixel_data *pixel_data, FILE *fp)
{
    png_structp png_ptr = NULL;
    png_infop info_ptr = NULL;

    if (pixel_data == NULL) goto_error(ERR_INVALID_PIXELDATA, "The provided pixel_data is NULL");
    if (pixel_data->w <= 0) goto_error(ERR_INVALID_WIDTH, "pixel_data->w is invalid (%u)", pixel_data->w);
    if (pixel_data->h <= 0) goto_error(ERR_INVALID_HEIGHT, "pixel_data->h is invalid (%u)", pixel_data->h);

    /* Initialize PNG output structures and file I/O */
    if (fp == NULL)
        goto_error(ERR_OUTPUT_FILEP_NULL, "The out file pointer is NULL");

    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (png_ptr == NULL)
        goto_error(ERR_INIT_PNG_WRITE_STRUCT, "Failed to create the PNG write struct!");

    info_ptr = png_create_info_struct(png_ptr);
    if (info_ptr == NULL)
        goto_error(ERR_INIT_PNG_WRITE_INFO, "Failed to create the PNG write info struct!");

    png_init_io(png_ptr, fp);

    /* Set the output image format to standard TrueColor (RGBA8888) */
    png_set_IHDR(png_ptr, info_ptr, pixel_data->w, pixel_data->h,
            8, PNG_COLOR_TYPE_RGBA, PNG_INTERLACE_NONE,
            PNG_COMPRESSION_TYPE_BASE, PNG_NO_FILTERS);

    /* Bind our modified raw data to the PNG write structs */
    png_set_rows(png_ptr, info_ptr, pixel_data->data);

    /* And we're done! */
    png_write_png(png_ptr, info_ptr, 0, NULL);

    /* Now just clean up. */
    png_destroy_write_struct(&png_ptr, &info_ptr);

    return EXIT_SUCCESS;

err_l:
    if (info_ptr) png_destroy_info_struct(png_ptr, &info_ptr);
    if (png_ptr) png_destroy_write_struct(&png_ptr, NULL);

    return write_PNG_error_code + 1; /* The codes start at 0 */
}
