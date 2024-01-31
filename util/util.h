#ifndef _UTIL_H
#define _UTIL_H

#include <SDL2/SDL.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/cdefs.h>

#define u_BUF_SIZE  1024
#define u_PATH_FROM_BIN_TO_ASSETS "../assets"

typedef struct {
    SDL_Rect srcRect, destRect, hitbox;
    SDL_Texture* texture;
} u_Sprite;

typedef struct {
    uint8_t r, g, b, a;
} u_Color;

const char *u_getAssetPath();
int u_max(int a, int b);
int u_min(int a, int b);
bool u_collision(const SDL_Rect *r1, const SDL_Rect *r2);
void u_error(const char *fmt, ...);
int u_getBinDir(char *buf, size_t buf_size);

#endif /* _UTIL_H */
