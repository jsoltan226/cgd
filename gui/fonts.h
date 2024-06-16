#ifndef FONTS_H
#define FONTS_H

#include "core/shapes.h"
#include "core/int.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_pixels.h>
#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_video.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <ft2build.h>
#include FT_FREETYPE_H

#define FNT_ASCII_FIRST_VISIBLE_CHAR        (((u32)' ') + 1)
#define FNT_ASCII_LAST_VISIBLE_CHAR         127
#define FNT_ASCII_TOTAL_VISIBLE_CHARS       (FNT_ASCII_LAST_VISIBLE_CHAR - FNT_ASCII_FIRST_VISIBLE_CHAR + 1)

#define FNT_DEFAULT_TAB_WIDTH       4
#define FNT_DEFAULT_TEXT_COLOR      (color_RGBA32_t){ 0, 0, 0, 255 }
#define FNT_TEXT_BUFFER_SIZE        1024

enum font_charset {
    FNT_CHARSET_ASCII,
    FNT_CHARSET_UTF8,
};

enum font_flag {
    FNT_FLAG_DISPLAY_CHAR_RECTS     = 1,
    FNT_FLAG_DISPLAY_GLYPH_RECTS    = 2 << 0,
    FNT_FLAG_DISPLAY_TEXT_RECTS     = 2 << 1,
};

struct font_glyph_data {
    rect_t src_rect;
    f32 offsetX, offsetY;
    f32 scaleX, scaleY;
};

struct font {
    SDL_Texture *texture;

    struct font_glyph_data *glyphs;
    struct {
        u32 first; u32 last; u32 total;
    } visible_chars;
    enum font_charset charset;

    f32 line_height;
    f32 charW;
    u16 tab_width;

    u16 flags;
};

struct font * font_init(const char *filepath, SDL_Renderer *renderer, f32 charW, f32 charH,
        enum font_charset charset, u16 flags);

i32 font_draw_text(struct font *font, SDL_Renderer *renderer, vec2d_t *pos, const char *fmt, ...);

void font_destroy(struct font *font);

/* Sets the font's texture color- and blend mode */
void font_set_text_color(struct font *font, u8 r, u8 g, u8 b, u8 a);

void font_set_line_height(struct font *font, f32 line_height);
void font_set_char_width(struct font *font, f32 charW);
void font_set_tab_width(struct font *font, u16 tab_width);

#endif /* FONTS_H */
