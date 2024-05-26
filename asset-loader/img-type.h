#ifndef IMG_TYPE_H_
#define IMG_TYPE_H_

#include <stdio.h>

enum asset_img_type {
    IMG_TYPE_UNKNOWN = 0,
    IMG_TYPE_PNG,
};

enum asset_img_type asset_get_img_type(FILE *fp);

#endif /* IMG_TYPE_H_ */
