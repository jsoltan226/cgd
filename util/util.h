#ifndef _UTIL_H
#define _UTIL_H

#include <SDL2/SDL.h>
#include <stdint.h>
#include <stdbool.h>
#include "int.h"
#include "shapes.h"

#define u_BUF_SIZE  1024
#define u_PATH_FROM_BIN_TO_ASSETS "../assets/"

typedef struct {
    rect_t srcRect, destRect, hitbox;
    SDL_Texture* texture;
} u_Sprite;

typedef struct {
    u8 r, g, b, a;
} u_Color;

const char *u_get_asset_dir();
i64 u_max(i64 a, i64 b);
i64 u_min(i64 a, i64 b);
bool u_collision(const rect_t *r1, const rect_t *r2);
void u_error(const char *fmt, ...);
i32 u_get_bin_dir(char *buf, u32 buf_size);

#endif /* _UTIL_H */
