#ifndef FONTS_H
#define FONTS_H

#include <cgd/core/shapes.h>
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

#define FNT_ASCII_FIRST_VISIBLE_CHAR        (((u32)' ') + 1)
#define FNT_ASCII_LAST_VISIBLE_CHAR         127
#define FNT_ASCII_TOTAL_VISIBLE_CHARS       (FNT_ASCII_LAST_VISIBLE_CHAR - FNT_ASCII_FIRST_VISIBLE_CHAR + 1)

#define FNT_DEFAULT_TAB_WIDTH       4
#define FNT_DEFAULT_TEXT_COLOR      (color_RGBA32_t){ 0, 0, 0, 255 }
#define FNT_TEXT_BUFFER_SIZE        1024

typedef enum {
    FNT_CHARSET_ASCII,
    FNT_CHARSET_UTF8,
} fnt_Charset;

typedef enum {
    FNT_FLAG_DISPLAY_CHAR_RECTS     = 1,
    FNT_FLAG_DISPLAY_GLYPH_RECTS    = 2 << 0,
    FNT_FLAG_DISPLAY_TEXT_RECTS     = 2 << 1,
} fnt_Flag;

typedef struct {
    rect_t srcRect;
    f32 offsetX, offsetY;
    f32 scaleX, scaleY;
} fnt_GlyphData;

typedef struct {
    /* Used internally, not modifiable */
    SDL_Texture *texture;

    fnt_GlyphData *glyphs;
    struct {
        u32 first; u32 last; u32 total;
    } visibleChars;
    fnt_Charset charset;

    /* User-modifiable at runtime */
    f32 lineHeight;
    f32 charW;
    u16 tabWidth;

    u16 flags;
} fnt_Font;

fnt_Font *fnt_initFont(const char *filePath, SDL_Renderer *renderer, f32 charW, f32 charH,
        fnt_Charset charset, u16 flags);

int fnt_renderText(fnt_Font *fnt, SDL_Renderer *renderer, vec2d_t *pos, const char *fmt, ...);

void fnt_destroyFont(fnt_Font *fnt);

/* Sets the font's texture color- and blend mode */
void fnt_setTextColor(fnt_Font *fnt, u8 r, u8 g, u8 b, u8 a);

#endif /* FONTS_H */
