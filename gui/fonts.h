#ifndef FONTS_H
#define FONTS_H

#include <cgd/util/datastruct/hashtable.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_pixels.h>
#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_video.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <ft2build.h>
#include <sys/types.h>
#include FT_FREETYPE_H

#define FNT_ASCII_FIRST_VISIBLE_CHAR        (((int)' ') + 1)
#define FNT_ASCII_LAST_VISIBLE_CHAR         127
#define FNT_ASCII_TOTAL_VISIBLE_CHARS       (FNT_ASCII_LAST_VISIBLE_CHAR - FNT_ASCII_FIRST_VISIBLE_CHAR + 1)

#define FNT_DEFAULT_TAB_WIDTH       4
#define FNT_DEFAULT_TEXT_COLOR      (SDL_Color){ 0, 0, 0, 255 }
#define FNT_TEXT_BUFFER_SIZE        1024

typedef uint64_t fnt_uInt64;
typedef uint32_t fnt_uInt32;
typedef uint16_t fnt_uInt16;
typedef uint8_t fnt_uInt8;
typedef float_t fnt_float;
typedef double_t fnt_double;

typedef enum {
    FNT_CHARSET_ASCII,
    FNT_CHARSET_UTF8,
} fnt_Charset;

typedef struct {
    fnt_float x, y;
} fnt_Vector2D;

typedef enum {
    FNT_FLAG_DISPLAY_CHAR_RECTS     = 1,
    FNT_FLAG_DISPLAY_GLYPH_RECTS    = 2 << 0,
    FNT_FLAG_DISPLAY_TEXT_RECTS     = 2 << 1,
} fnt_Flag;

typedef struct {
    SDL_Rect srcRect;
    fnt_float offsetX, offsetY;
    fnt_float scaleX, scaleY;
} fnt_GlyphData;

typedef struct {
    SDL_Texture *texture;
} fnt_CacheEntry;

typedef struct {
    ht_HashTable *textureTable;
    char (*cachedStrings)[FNT_TEXT_BUFFER_SIZE];
    int n_cachedStrings;
} fnt_Cache;

typedef struct {
    /* Used internally, not modifiable */
    SDL_Texture *texture;

    fnt_GlyphData *glyphs;
    struct {
        fnt_uInt32 first;
        fnt_uInt32 last;
        fnt_uInt32 total;
    } visibleChars;
    fnt_Charset charset;

    fnt_Cache cache;

    /* User-modifiable at runtime */
    fnt_float lineHeight;
    fnt_float charW;
    fnt_uInt16 tabWidth;

    fnt_uInt16 flags;
} fnt_Font;

fnt_Font *fnt_initFont(const char *filePath, SDL_Renderer *renderer, fnt_float charW, fnt_float charH, 
        fnt_Charset charset, fnt_uInt16 flags);

SDL_Texture *fnt_renderText(fnt_Font *fnt, SDL_Renderer *renderer, fnt_Vector2D *pos, const char *fmt, ...);

void fnt_destroyFont(fnt_Font *fnt);

/* Sets the font's texture color- and blend mode */
void fnt_setTextColor(fnt_Font *fnt, fnt_uInt8 r, fnt_uInt8 g, fnt_uInt8 b, fnt_uInt8 a);

#endif /* FONTS_H */
