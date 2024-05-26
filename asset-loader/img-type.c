#include "img-type.h"
#include <cgd/util/int.h>
#include <png.h>
#include <stdio.h>
#include <string.h>

enum asset_img_type asset_get_img_type(FILE *fp)
{
#define PNG_MAGIC_SIZE 8

    u8 header[PNG_MAGIC_SIZE];

    if (fp == NULL) return IMG_TYPE_UNKNOWN;

    /* Read PNG header to check whether the given file contains a valid PNG signature */
    if (fread(header, 1, PNG_MAGIC_SIZE, fp) < PNG_MAGIC_SIZE) return IMG_TYPE_UNKNOWN;

    if (png_sig_cmp(header, 0, PNG_MAGIC_SIZE)) return IMG_TYPE_UNKNOWN;

    fseek(fp, 0, SEEK_SET); /* Move the file pointer back to the start of the file */

    return IMG_TYPE_PNG;
}
