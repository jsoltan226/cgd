#include "fonts.h"
#include "core/math.h"
#include "core/util.h"
#include "core/log.h"
#include <SDL2/SDL_error.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_blendmode.h>
#include <stdlib.h>
#include <string.h>

#define goto_error(msg...) do {     \
    s_log_error("font", msg);       \
    goto err;                       \
} while (0);

struct font * font_init(const char *filepath, SDL_Renderer *renderer, f32 charW, f32 lineH,
        enum font_charset charset, u16 flags)
{
    if (filepath == NULL || renderer == NULL) {
        s_log_error("font", "Invalid parameters");
        return NULL;
    }

    /* The exit error codes used by the 'err' label */
    i32 ft_ret = 0;

    char c = 0; /* Used later, but the declaration must be here, as it's needed by the 'err' label */
    struct font *new_font = NULL;
    SDL_Surface **tmp_surfaces = NULL;
    SDL_Surface *final_surface = NULL;
    FT_Library ft = NULL;
    FT_Face face = NULL;

    new_font = calloc(1, sizeof(struct font));
    if(new_font == NULL)
        goto_error("malloc() failed for struct font!");

    s_log_debug("font", "Initializing FreeType...");
    /* Initialize FreeType and load the font */
    ft = NULL;
    if(ft_ret = FT_Init_FreeType(&ft), ft_ret != 0)
        goto_error("Failed to initialize FreeType: %s", FT_Error_String(ft_ret));

    char full_filepath[u_BUF_SIZE] = { 0 };
    strncpy(full_filepath, u_get_asset_dir(), u_BUF_SIZE - 1);
    strncat(full_filepath, filepath, u_BUF_SIZE - strlen(full_filepath) - 1);

    s_log_debug("font", "Initializing font face from \"%s\"...", full_filepath);

    face = NULL;
    if (ft_ret = FT_New_Face(ft, full_filepath, 0, &face), ft_ret != 0)
        goto_error("Failed to initialize FreeType face for \"%s\": %s",
            filepath, FT_Error_String(ft_ret));

    if (ft_ret = FT_Set_Pixel_Sizes(face, 0, lineH), ft_ret != 0)
        goto_error("Failed to set FreeType pixel sizes with lineH %i: %s",
            lineH, FT_Error_String(ft_ret));

    /* Populate the new font struct with the given/default data */
    new_font->charW = charW > 0 ? charW : face->size->metrics.max_advance >> 6;
    new_font->line_height = lineH;
    new_font->charset = charset;
    new_font->tab_width = FNT_DEFAULT_TAB_WIDTH;
    new_font->flags = flags;

    switch (charset) {
        default: case FNT_CHARSET_ASCII:
            s_log_debug("font", "Charset is ASCII");
            new_font->visible_chars.first = FNT_ASCII_FIRST_VISIBLE_CHAR;
            new_font->visible_chars.last = FNT_ASCII_LAST_VISIBLE_CHAR;
            new_font->visible_chars.total = FNT_ASCII_TOTAL_VISIBLE_CHARS;
            break;
        case FNT_CHARSET_UTF8:
            // temporary
            s_log_debug("font", "Charset is UTF-8");
            new_font->visible_chars.first = FNT_ASCII_FIRST_VISIBLE_CHAR;
            new_font->visible_chars.last = FNT_ASCII_LAST_VISIBLE_CHAR;
            new_font->visible_chars.total = FNT_ASCII_TOTAL_VISIBLE_CHARS;
            break;
    };

    new_font->glyphs = calloc(new_font->visible_chars.total, sizeof(struct font_glyph_data));
    if (new_font->glyphs == NULL)
        goto_error("calloc() failed for font glyphs!");

    /* Prepare the temporary surfaces and the total width and height,
     * used later to create the final surface with the appropriate dimensions */
    tmp_surfaces = calloc(new_font->visible_chars.total, sizeof(SDL_Surface *));
    if (tmp_surfaces == NULL) goto_error("malloc() failed for tmp_surfaces!");

    u32 totalW = 0, totalH = 0;

    for(u32 i = 0; i < new_font->visible_chars.total; i++){
        struct font_glyph_data *curr_glyph = &new_font->glyphs[i];

        /* char */ c = (char)(i + new_font->visible_chars.first);
        if(ft_ret = FT_Load_Char(face, c, FT_LOAD_RENDER), ft_ret != 0)
            goto_error("Failed to load char '%c' (0x%x): %s", c, c, FT_Error_String(ft_ret));

        FT_GlyphSlot glyph_slot = face->glyph;

        /* Set the glyph's source rect to based on the metadata from the FT_GlyphSlot struct
         * and the current total width (which also happens to be where our 'pen' is on the X axis)
         */
        curr_glyph->src_rect = (rect_t){
            .x = totalW,
            .y = 0,
            .w = glyph_slot->bitmap.width,
            .h = glyph_slot->bitmap.rows,
        };

        /* For some reason FreeType uses 1/64th of a pixel as its metrics unit,
         * so to obtain our wanted result we multiply everything by 64 (shift 6 bits to the right)
         */

        FT_Glyph_Metrics *m = &glyph_slot->metrics;
        if(m->horiAdvance != 0)
            curr_glyph->scaleX = (f32)m->width / m->horiAdvance;

        if((f32)((i32)lineH << 6) != 0)
            curr_glyph->scaleY = (f32)m->height / (f32)((i32)lineH << 6);

        if(m->horiAdvance != 0)
            curr_glyph->offsetX = (f32)m->horiBearingX / m->horiAdvance;

        /* It took me 5 days to come up with this one line,
         * so I don't think you should bother trying to understand it yourself.
         * You (probably) have the luxury to only need to know WHAT this does, not HOW... */
        if(lineH != 0)
            curr_glyph->offsetY = 1.f - ((f32)m->horiBearingY - face->descender) / (f32)((i32)lineH << 6);

        /* The font's texture will be a long (horizontal) line
         * with all the character placed one after the other,
         * so we increment the total width by the current glyph's width,
         * and only set the height to be the one of the taller character
         */
        totalW += curr_glyph->src_rect.w;
        totalH = max(totalH, glyph_slot->metrics.height >> 6);

        tmp_surfaces[i] = SDL_CreateRGBSurfaceWithFormat(
            0, glyph_slot->bitmap.width, glyph_slot->bitmap.rows, 32, SDL_PIXELFORMAT_RGBA32
        );
        SDL_Surface *curr_surface = tmp_surfaces[i];
        if(curr_surface == NULL)
            goto_error("Failed to create SDL Surface @ index %i: %s", i, SDL_GetError());

        /* Fill the surface with a transparent black (RGBA 0 0 0 0) background */
        SDL_LockSurface(curr_surface);
        SDL_FillRect(curr_surface, NULL, SDL_MapRGBA(curr_surface->format, 0, 0, 0, 0));

        /* Go through all the pixels one by one,
         * and map them over onto the current glyph's temp surface */
        for (u32 y = 0; y < glyph_slot->bitmap.rows; ++y) {
            for (u32 x = 0; x < glyph_slot->bitmap.width; ++x) {
                u8 pixel = glyph_slot->bitmap.buffer[y * glyph_slot->bitmap.width + x];
                u32* target_pixel = (u32*)((u8*)curr_surface->pixels + y * curr_surface->pitch + x * sizeof(u32));
                *target_pixel = SDL_MapRGBA(curr_surface->format, pixel, pixel, pixel, pixel);
            }
        }
        SDL_UnlockSurface(curr_surface);
    }

    FT_Done_Face(face);
    FT_Done_FreeType(ft);

    s_log_debug("font", "Total width is %i, total height is %i. Creating final surface...",
        totalW, totalH);
    final_surface = SDL_CreateRGBSurfaceWithFormat(
        0, totalW, totalH, 32, SDL_PIXELFORMAT_RGBA32
    );
    if (final_surface == NULL)
        goto_error("Failed to create final surface with totalW: %i, totalH: %i: %s",
            totalW, totalH, SDL_GetError());

    /* Iterate through the temporary surfaces and draw each of them,
     * one by one, onto the final surface */
    for(u32 i = 0; i < new_font->visible_chars.total; i++){
        SDL_BlitSurface(tmp_surfaces[i], NULL, final_surface, (SDL_Rect *)&new_font->glyphs[i].src_rect);

        SDL_FreeSurface(tmp_surfaces[i]);
        tmp_surfaces[i] = NULL;
    }
    free(tmp_surfaces);
    tmp_surfaces = NULL;

    /* Now, create the new font's texture from the final temp surface */
    new_font->texture = SDL_CreateTextureFromSurface(renderer, final_surface);
    if (new_font->texture == NULL)
        goto_error("Failed to create the final font texture: %s", SDL_GetError());

    SDL_FreeSurface(final_surface);

    SDL_SetTextureBlendMode(new_font->texture, SDL_BLENDMODE_BLEND);
    font_set_text_color(new_font, u_color_arg_expand(FNT_DEFAULT_TEXT_COLOR));

    s_log_debug("font", "OK loading font from \"%s\"", filepath);

    return new_font;

err:
    if (face) FT_Done_Face(face);
    if (ft) FT_Done_FreeType(ft);

    if (new_font) {
        if(tmp_surfaces) {
            for (u32 i = 0; i < new_font->visible_chars.total; i++) {
                if (tmp_surfaces[i]) SDL_FreeSurface(tmp_surfaces[i]);
            }
            free(tmp_surfaces);
        }
        if (final_surface) SDL_FreeSurface(final_surface);

        if (new_font->texture) SDL_DestroyTexture(new_font->texture);
        if (new_font->glyphs) free(new_font->glyphs);
        free(new_font);
    }
    return NULL;
}

i32 font_draw_text(struct font *fnt, SDL_Renderer *renderer, vec2d_t *pos, const char *fmt, ...)
{
    if (fnt == NULL || renderer == NULL || pos == NULL) return -1;

    va_list vArgs;
    char str[FNT_TEXT_BUFFER_SIZE] = { 0 };

    va_start(vArgs, fmt);
    vsnprintf(str, FNT_TEXT_BUFFER_SIZE - 1, fmt, vArgs);
    va_end(vArgs);
    str[FNT_TEXT_BUFFER_SIZE - 1] = '\0';

    /* penX and penY are relative to the given baseline coorinates */
    i32 penX = 0, penY = 0;
    i32 maxPenX = 0, maxCharH = 0;

    const char *c_ptr = str;
    u32 i;

    c_ptr = str;
    while(*c_ptr){
        switch(*c_ptr){
            default:
                i = (u32)(*c_ptr) - fnt->visible_chars.first;
                if(i < fnt->visible_chars.total && i >= 0){
                    struct font_glyph_data *curr_glyph = &fnt->glyphs[i];

                    rect_t glyphDestRect = {
                        .x = pos->x + penX + (curr_glyph->offsetX * fnt->charW),
                        .y = pos->y + penY + (curr_glyph->offsetY * fnt->line_height),
                        .w = (i32)(fnt->charW * curr_glyph->scaleX),
                        .h = (i32)(fnt->line_height * curr_glyph->scaleY),
                    };
                    maxCharH = max(maxCharH, glyphDestRect.h | (i64)fnt->line_height);

                    SDL_RenderCopy(renderer, fnt->texture,
                        (const SDL_Rect *)&fnt->glyphs[i].src_rect,
                        (const SDL_Rect *)&glyphDestRect
                    );
                    if(fnt->flags & FNT_FLAG_DISPLAY_GLYPH_RECTS){
                        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
                        SDL_RenderDrawRect(renderer, (const SDL_Rect *)&glyphDestRect);
                    }

                    rect_t charDestRect = (rect_t){
                        .x = pos->x + penX,
                        .y = pos->y + penY,
                        .w = fnt->charW,
                        .h = fnt->line_height
                    };
                    if(fnt->flags & FNT_FLAG_DISPLAY_CHAR_RECTS){
                        SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
                        SDL_RenderDrawRect(renderer, (const SDL_Rect *)&charDestRect);
                    }

                    penX += fnt->charW;
                }
                break;
            case ' ':
                penX += fnt->charW;
                break;
            case '\r': /* Carriage return */
                penX = 0;
                break;
            case '\t':
                /* Advance to the next multiple of tab_width (times the character width) */
                if(fnt->charW != 0)
                    penX += (fnt->tab_width -((i32)(penX / fnt->charW) % fnt->tab_width)) * fnt->charW;
                break;
            case '\n':
                maxPenX = max(penX, maxPenX);
                penX = 0;
                penY += fnt->line_height;
                break;
            case '\f': /* Form feed character, just treat it as two newline characters */
                penX = 0;
                penY += fnt->line_height * 2;
                break;
            case '\v': /* Vertical tab (newline + tab) */
                penY += fnt->line_height;
                maxPenX = max(penX, maxPenX);
                penX = fnt->tab_width * fnt->charW;
                break;
        }
        c_ptr++;
    }
    maxPenX = max(penX, maxPenX);

    if (fnt->flags & FNT_FLAG_DISPLAY_TEXT_RECTS) {
        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
        SDL_RenderDrawRect(renderer, &(SDL_Rect) {
                .x = pos->x - 1,
                .y = pos->y - 1,
                .w = maxPenX + 2,
                .h = penY + maxCharH + 2
            } );
    }

    return 0;
}

void font_destroy(struct font *font)
{
    if (font == NULL) return;
    s_log_debug("font", "Destroying font...");

    free(font->glyphs);
    if (font->texture != NULL)
        SDL_DestroyTexture(font->texture);

    free(font);
}

void font_set_text_color(struct font *font, u8 r, u8 g, u8 b, u8 a)
{
    if (font == NULL) return;
    SDL_SetTextureColorMod(font->texture, r, g, b);
    SDL_SetTextureAlphaMod(font->texture, a);
}

void font_set_line_height(struct font *font, f32 line_height)
{
    if (font != NULL)
        font->line_height = line_height;
}

void font_set_char_width(struct font *font, f32 charW)
{
    if (font != NULL)
        font->charW = charW;
}

void font_set_tab_width(struct font *font, u16 tab_width)
{
    if (font != NULL)
        font->tab_width = tab_width;
}
