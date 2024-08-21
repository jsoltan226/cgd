#ifndef IMG_TYPE_H_
#define IMG_TYPE_H_

#include <stdio.h>
#include "core/util.h"

#define ASSET_IMG_TYPES_LIST    \
    X_(IMG_TYPE_UNKNOWN)        \
    X_(IMG_TYPE_PNG)            \

#define X_(name) name,
enum asset_img_type {
    ASSET_IMG_TYPES_LIST
};
#undef X_

#define X_(name) #name,
static const char *const asset_img_type_strings[] = {
    ASSET_IMG_TYPES_LIST
};
#undef X_

#define asset_get_type_name(type)                               \
    ((type < u_arr_size(asset_img_type_strings) && type >= 0)   \
     ? asset_img_type_strings[type] : NULL)

enum asset_img_type asset_get_img_type(FILE *fp);

#endif /* IMG_TYPE_H_ */
