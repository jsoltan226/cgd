#include "img-type.h"
#include "io-PNG.h"
#include <core/log.h>
#include <stdio.h>
#include <string.h>

#define MODULE_NAME "assetld"

enum asset_img_type asset_get_img_type(FILE *fp)
{
    if (fp == NULL) {
        s_log_error("The provided file pointer is NULL!");
        return IMG_TYPE_UNKNOWN;
    }

    if (!is_PNG(fp)) {
        s_log_error("The image is not a valid PNG, return IMG_TYPE_UNKNOWN!");
        return IMG_TYPE_UNKNOWN;
    }

    fseek(fp, 0, SEEK_SET); /* Move the file pointer back to the start of the file */

    s_log_debug("Image type is PNG");
    return IMG_TYPE_PNG;
}
