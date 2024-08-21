#include "io-PNG.h"
#include "core/librtld.h"
#include "core/log.h"
#include "core/int.h"
#include "core/pixel.h"
#include <err.h>
#include <limits.h>
#include <png.h>
#include <setjmp.h>
#include <stdlib.h>
#include <string.h>

#define MODULE_NAME "io-PNG"

#define LIBPNG_NAME "libpng.so"
#define LIBPNG_SYM_LIST                                                             \
    X_(int, png_sig_cmp, (png_bytep sig, size_t start, size_t num_to_check))        \
    X_(png_structp, png_create_read_struct,                                         \
        (png_const_charp user_png_ver, png_voidp error_ptr, png_error_ptr error_fn, \
         png_error_ptr warn_fn)                                                     \
    )                                                                               \
    X_(png_infop, png_create_info_struct, (png_structp png_ptr))                    \
    X_(jmp_buf *, png_set_longjmp_fn,                                               \
        (png_structrp png_ptr, png_longjmp_ptr longjmp_fn, size_t jmp_buf_size)     \
    )                                                                               \
    X_(void, png_init_io, (png_structp png_ptr, FILE *fp))                          \
    X_(void, png_set_sig_bytes, (png_structp png_ptr, int num_bytes))               \
    X_(void, png_read_info, (png_structp png_ptr, png_infop info_ptr))              \
    X_(png_uint_32, png_get_IHDR,                                                   \
        (png_structp  png_ptr, png_infop info_ptr, png_uint_32  *width,             \
         png_uint_32 *height, int *bit_depth, int *color_type, int *interlace_type, \
         int *compression_type, int *filter_type)                                   \
    )                                                                               \
    X_(png_uint_32, png_get_PLTE,                                                   \
        (png_const_structp png_ptr, png_const_infop info_ptr, png_colorp *palette,  \
         int *num_palette)                                                          \
    )                                                                               \
    X_(png_uint_32, png_get_tRNS,                                                   \
        (png_const_structp png_ptr, png_infop info_ptr, png_bytep *trans_alpha,     \
         int *num_trans, png_color_16p *trans_color)                                \
    )                                                                               \
    X_(png_uint_32, png_get_gAMA,                                                   \
        (png_const_structp png_ptr, png_const_infop info_ptr, double *file_gamma)   \
    )                                                                               \
    X_(void, png_set_strip_16, (png_structp png_ptr))                               \
    X_(void, png_set_palette_to_rgb, (png_structp png_ptr))                         \
    X_(void, png_set_gray_to_rgb, (png_structp png_ptr))                            \
    X_(void, png_set_expand_gray_1_2_4_to_8, (png_structp png_ptr))                 \
    X_(png_uint_32, png_get_valid,                                                  \
        (png_const_structp png_ptr, png_const_infop info_ptr, png_uint_32 flag)     \
    )                                                                               \
    X_(void, png_set_tRNS_to_alpha, (png_structp png_ptr))                          \
    X_(void, png_set_filler, (png_structp png_ptr, png_uint_32 filler, int flags))  \
    X_(void, png_read_update_info, (png_structp png_ptr, png_infop info_ptr))       \
    X_(void, png_read_image, (png_structp png_ptr, png_bytepp image))               \
    X_(void, png_destroy_read_struct,                                               \
        (png_structpp png_ptr_ptr, png_infopp info_ptr_ptr,                         \
         png_infopp end_info_ptr_ptr)                                               \
    )                                                                               \
    X_(void, png_destroy_info_struct,                                               \
        (png_structp png_ptr, png_infopp info_ptr_ptr)                              \
    )                                                                               \
    X_(png_structp, png_create_write_struct,                                        \
        (png_const_charp user_png_ver, png_voidp error_ptr,                         \
         png_error_ptr error_fn, png_error_ptr warn_fn)                             \
    )                                                                               \
    X_(void, png_set_IHDR,                                                          \
        (png_structp png_ptr, png_infop info_ptr, png_uint_32 width,                \
         png_uint_32  height, int bit_depth, int color_type, int interlace_type,    \
         int compression_type, int filter_type)                                     \
    )                                                                               \
    X_(void, png_set_rows,                                                          \
        (png_structp png_ptr, png_infop info_ptr, png_bytepp row_pointers)          \
    )                                                                               \
    X_(void, png_write_png,                                                         \
        (png_structp png_ptr, png_infop info_ptr, int transforms, png_voidp params) \
    )                                                                               \
    X_(void, png_destroy_write_struct,                                              \
        (png_structpp png_ptr_ptr, png_infopp info_ptr_ptr)                         \
    )                                                                               \

static struct lib *libPNG = NULL;

#define X_(ret_type, name, ...) \
    ret_type (*name) __VA_ARGS__;
static struct libPNG_syms {
    LIBPNG_SYM_LIST
} PNG = { 0 };
#undef X_

i32 load_libPNG();

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
    ERR_LIBPNG_NOT_LOADED_r,
    ERR_INVALID_ARGS,
    ERR_NOT_PNG,
    ERR_INIT_PNG_STRUCT,
    ERR_INIT_PNG_READ_INFO,
    ERR_INIT_PNG_END,
    ERR_SETJMP,
} load_PNG_error_code;

#undef goto_error
#define goto_error(code, msg...) do { \
    load_PNG_error_code = code; \
    s_log_error(msg); \
    goto err_cleanup_l; \
} while (0);

#define N_PNG_SIG_BYTES 8
#define N_CHANNELS 4
i32 read_PNG(struct pixel_flat_data *pixel_data, FILE *fp)
{
    if (libPNG == NULL) {
        s_log_error("Attempt to read PNG image before loading libPNG");
        return ERR_LIBPNG_NOT_LOADED_r;
    }

    png_structp png_ptr = NULL;
    png_infop info_ptr = NULL;
    png_infop end_ptr = NULL;

    struct PNG_Metadata meta;

    if (pixel_data == NULL || fp == NULL)
        goto_error(ERR_INVALID_ARGS, "read_png: Invalid parameters");

    struct pixel_row_data img_row_data = { 0 };

    /* Read the PNG header to check whether the given file contains a valid PNG signature */
    if (!is_PNG(fp))
        goto_error(ERR_NOT_PNG, "The file is not a valid PNG (magic bytes missing)!");

    /** Initialization and setup of libpng structures **/
    /* Initialize png struct */
    png_ptr = PNG.png_create_read_struct(PNG_LIBPNG_VER_STRING,
        NULL, NULL, NULL
    );
    if (png_ptr == NULL)
        goto_error(ERR_INIT_PNG_STRUCT, "Failed to create the read PNG struct");

    /* Initialize png info struct */
    info_ptr = PNG.png_create_info_struct(png_ptr);
    if (info_ptr == NULL)
        goto_error(ERR_INIT_PNG_READ_INFO, "Failed to create the read PNG info struct");

    /* Not creating the end_ptr sometimes caused a weird memory error I don't understand */
    end_ptr = PNG.png_create_info_struct(png_ptr);
    if (end_ptr == NULL)
        goto_error(ERR_INIT_PNG_END, "Failed to create the read PNG end info struct");

    /* Setting up libpng error handling */
    if (setjmp(*(PNG.png_set_longjmp_fn(png_ptr, longjmp, sizeof(jmp_buf)))))
        goto_error(ERR_SETJMP, "setjmp() failed while setting up PNG error handling");

    /* Set up the PNG input code */
    PNG.png_init_io(png_ptr, fp);

    /* Inform libpng that we have read the signature bytes */
    PNG.png_set_sig_bytes(png_ptr, N_PNG_SIG_BYTES);

    /** Read image metadata **/
    PNG.png_read_info(png_ptr, info_ptr);
    PNG.png_get_IHDR(png_ptr, info_ptr, &meta.width, &meta.height, &meta.bitDepth, &meta.colorType,
            &meta.interlaceMethod, &meta.compressionMethod, &meta.filterMethod);

    PNG.png_get_PLTE(png_ptr, info_ptr, &meta.palette, &meta.n_palette);

    PNG.png_get_tRNS(png_ptr, info_ptr, &meta.trans_alpha, &meta.n_trans, &meta.trans_color);

    PNG.png_get_gAMA(png_ptr, info_ptr, &meta.gamma);

    /** Make sure the raw image data is transformed
     ** to standard RGBA8888 (TrueColor) format
     **/
    /* And yes, any other self-respecting library would do that automatically with a sigle function call... */

    /* Make sure each color channel is 8 bits (as it could be 16) */
    if (meta.bitDepth == 16)
        PNG.png_set_strip_16(png_ptr);

    /* PLTE (Pallette) is an alternative method of storing color values.
     * We need to make sure to transform it to our standard RGBA format */
    if (meta.colorType == PNG_COLOR_TYPE_PALETTE)
        PNG.png_set_palette_to_rgb(png_ptr);

    /* PNG supports grayscale images
     * (Just one gray (G) channel instead of three for RGB).
     * We obviously need to transofrm this to RGB
     */
    if (meta.colorType == PNG_COLOR_TYPE_GRAY || meta.colorType == PNG_COLOR_TYPE_GRAY_ALPHA)
        PNG.png_set_gray_to_rgb(png_ptr);

    /* Again, make sure that the amount of bits per channel is 8 */
    if (meta.colorType == PNG_COLOR_TYPE_GRAY && meta.bitDepth < 8)
        PNG.png_set_expand_gray_1_2_4_to_8(png_ptr);

    /* tRNS (Transparency) is an alternative way of storing the alpha channel.
     * We need to transform this to fit the RGBA format */
    if (PNG.png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) {
        PNG.png_set_tRNS_to_alpha(png_ptr);
    } else if (meta.colorType == PNG_COLOR_TYPE_RGB ||
        meta.colorType == PNG_COLOR_TYPE_GRAY ||
        meta.colorType == PNG_COLOR_TYPE_PALETTE)
    {
    /* If no transparency is found,
     * fill the missing alpha channel with 0xff (fully opaque) */
        PNG.png_set_filler(png_ptr, 0xff, PNG_FILLER_AFTER);
    }

    /* Apply all transformations needed to get the raw data in RGBA8888 */
    PNG.png_read_update_info(png_ptr, info_ptr);

    /* Allocate memory for the raw pixel data */
    pixel_row_data_init(&img_row_data, meta.width, meta.height);

    /* And now, read the pixel data!!! FINALLY!!! */
    PNG.png_read_image(png_ptr, (u8 **)img_row_data.rows);

    /* Convert the rows to a flat array */
    pixel_data_row2flat(&img_row_data, pixel_data);

    /* Cleanup */
    pixel_row_data_destroy(&img_row_data);
    PNG.png_destroy_read_struct(&png_ptr, &info_ptr, &end_ptr);

    return EXIT_SUCCESS;

err_cleanup_l:
    if (end_ptr) PNG.png_destroy_info_struct(png_ptr, &end_ptr);
    if (info_ptr) PNG.png_destroy_info_struct(png_ptr, &info_ptr);
    if (png_ptr) PNG.png_destroy_read_struct(&png_ptr, NULL, NULL);
    pixel_row_data_destroy(&img_row_data);
    return load_PNG_error_code;
}

enum write_PNG_error_code {
    OK,
    ERR_LIBPNG_NOT_LOADED_w,
    ERR_OUTPUT_FILEP_NULL,
    ERR_INVALID_PIXELDATA,
    ERR_INVALID_WIDTH,
    ERR_INVALID_HEIGHT,
    ERR_INIT_PNG_WRITE_STRUCT,
    ERR_INIT_PNG_WRITE_INFO,
} write_PNG_error_code;

#undef goto_error
#define goto_error(code, msg...) do { \
    write_PNG_error_code = code; \
    s_log_error(msg); \
    goto err_l; \
} while (0);

i32 write_PNG(struct pixel_flat_data *pixel_data, FILE *fp)
{
    if (libPNG == NULL) {
        s_log_error("Attempt to write PNG image before loading libPNG");
        return ERR_LIBPNG_NOT_LOADED_w;
    }

    png_structp png_ptr = NULL;
    png_infop info_ptr = NULL;
    struct pixel_row_data img_row_data = { 0 };

    if (pixel_data == NULL)
        goto_error(ERR_INVALID_PIXELDATA, "The provided pixel_data is NULL");

    if (pixel_data->w <= 0)
        goto_error(ERR_INVALID_WIDTH, "pixel_data->w is invalid (%u)", pixel_data->w);

    if (pixel_data->h <= 0)
        goto_error(ERR_INVALID_HEIGHT, "pixel_data->h is invalid (%u)", pixel_data->h);

    /* Initialize PNG output structures and file I/O */
    if (fp == NULL)
        goto_error(ERR_OUTPUT_FILEP_NULL, "The out file pointer is NULL");

    png_ptr = PNG.png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (png_ptr == NULL)
        goto_error(ERR_INIT_PNG_WRITE_STRUCT, "Failed to create the PNG write struct!");

    info_ptr = PNG.png_create_info_struct(png_ptr);
    if (info_ptr == NULL)
        goto_error(ERR_INIT_PNG_WRITE_INFO, "Failed to create the PNG write info struct!");

    PNG.png_init_io(png_ptr, fp);

    /* Set the output image format to standard TrueColor (RGBA8888) */
    PNG.png_set_IHDR(png_ptr, info_ptr, pixel_data->w, pixel_data->h,
            8, PNG_COLOR_TYPE_RGBA, PNG_INTERLACE_NONE,
            PNG_COMPRESSION_TYPE_BASE, PNG_NO_FILTERS);

    /* Convert the flat array to rows */
    pixel_data_flat2row(pixel_data, &img_row_data);

    /* Bind our modified raw data to the PNG write structs */
    PNG.png_set_rows(png_ptr, info_ptr, (u8 **)img_row_data.rows);

    /* And we're done! */
    PNG.png_write_png(png_ptr, info_ptr, 0, NULL);

    /* Now just clean up. */
    PNG.png_destroy_write_struct(&png_ptr, &info_ptr);
    pixel_row_data_destroy(&img_row_data);

    return EXIT_SUCCESS;

err_l:
    if (info_ptr) PNG.png_destroy_info_struct(png_ptr, &info_ptr);
    if (png_ptr) PNG.png_destroy_write_struct(&png_ptr, NULL);

    return write_PNG_error_code;
}

i32 load_libPNG()
{
    if (libPNG != NULL) {
        s_log_warn("%s: libPNG already loaded!", __func__);
        return 0;
    }

#define X_(ret_type, name, ...) \
    #name,

    static const char *sym_names[] = {
        LIBPNG_SYM_LIST
        NULL
    };
#undef X_
    
    libPNG = librtld_load(LIBPNG_NAME, sym_names);
    if (libPNG == NULL)
        return 1;

#define X_(ret_type, name, ...) \
    PNG.name = librtld_get_sym_handle(libPNG, #name);

    LIBPNG_SYM_LIST

#undef X_

    return 0;
}

void close_libPNG()
{
    if (libPNG != NULL)
        librtld_close(libPNG);
    else
        s_log_warn("%s: libPNG already closed!");
}

i32 is_PNG(FILE *fp)
{
    static const u8 PNG_MAGIC[PNG_MAGIC_SIZE] = 
        { 0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a };
    
    u8 hdr_buf[PNG_MAGIC_SIZE] = { 0 };

    fread(hdr_buf, 1, PNG_MAGIC_SIZE, fp);
    return !(memcmp(hdr_buf, PNG_MAGIC, PNG_MAGIC_SIZE));
}
