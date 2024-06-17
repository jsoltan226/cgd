#ifndef _UTIL_H
#define _UTIL_H

#include <SDL2/SDL.h>
#include <stdbool.h>
#include "int.h"
#include "shapes.h"

#define u_BUF_SIZE  1024
#define u_PATH_FROM_BIN_TO_ASSETS "../assets/"

#define u_color_arg_expand(color) color.r, color.g, color.b, color.a

/* Pretty simple. Return the path to the asset directory
 * and the directory in which the binary is located, respectively */
const char * u_get_asset_dir();
i32 u_get_bin_dir(char *buf, u32 buf_size);

/* The simplest collision checking implementation;
 * returns true if 2 rectangles overlap
 */
bool u_collision(const rect_t *r1, const rect_t *r2);

#endif /* _UTIL_H */
