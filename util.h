#ifndef UTIL_H
#define UTIL_H

#include <SDL2/SDL.h>
#include <stdbool.h>
#include <sys/cdefs.h>

#define u_BUF_SIZE    1024
#define u_PATH_FROM_BIN_TO_ASSETS "../assets"

typedef struct {
    SDL_Rect srcRect, destRect, hitbox;
    SDL_Texture* texture;
} u_Sprite;

__attribute_maybe_unused__ static char _cgd_util_internal_binDirBuffer[u_BUF_SIZE];
__attribute_maybe_unused__ static char _cgd_util_internal_assetsDirBuffer[u_BUF_SIZE];

const char *u_getAssetPath();
SDL_Texture* u_loadPNG(const char* filePath, SDL_Renderer* renderer);
int u_max(int a, int b);
int u_min(int a, int b);
bool u_collision(const SDL_Rect *r1, const SDL_Rect *r2);
void u_error(const char *fmt, ...);
int u_getBinDir(char *buf, size_t buf_size);

#endif
