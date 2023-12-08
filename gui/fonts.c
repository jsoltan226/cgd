#include "fonts.h"
#include "freetype/freetype.h"
#include <SDL2/SDL_render.h>
#include <stdlib.h>

int max(int a, int b)
{
    if(a > b)
        return a;
    else
        return b;
};
int min(int a, int b)
{
    if(a < b)
        return a;
    else
        return b;
};
double fmax(double a, double b)
{
    if(a > b)
        return a;
    else
        return b;
};
double fmin(double a, double b)
{
    if(a < b)
        return a;
    else
        return b;
};

fnt_Font *fnt_initFont(const char *filePath, SDL_Renderer *renderer, fnt_float charW, fnt_float lineH, 
        fnt_Charset charset, fnt_uInt16 flags)
{
    /* The exit error codes used by the 'err' label */
    enum EXIT_CODES {
        EXIT_OK                     = 0,
        ERR_ALLOC_FNT_STRUCT        = 1,
        ERR_INIT_FREETYPE           = 2,
        ERR_INIT_FT_FACE            = 3,
        ERR_FT_SET_PIXEL_SIZES      = 4,
        ERR_ALLOC_FNT_GLYPHS        = 5,
        ERR_FT_LOAD_CHAR            = 7,
        ERR_CREATE_CHAR_SURFACE     = 8,
        ERR_CREATE_FINAL_SURFACE    = 9,
        ERR_CREATE_FNT_TEXTURE      = 10,
    };
    int errCode = EXIT_OK;
    int FreeType_errCode = 0;

    char c = 0; /* Used later, but the declaration must be here, as it's needed by the 'err' label */

    fnt_Font *newFont = malloc(sizeof(fnt_Font));
    if(newFont == NULL){
        errCode = ERR_ALLOC_FNT_STRUCT;
        goto err;
    }

    /* Initialize FreeType and load the font */
    FT_Library ft;
    FreeType_errCode = FT_Init_FreeType(&ft);
    if(FreeType_errCode){
        errCode = ERR_INIT_FREETYPE;
        goto err;
    }
    FT_Face face;
    FreeType_errCode = FT_New_Face(ft, filePath, 0, &face);
    if(FreeType_errCode){
        errCode = ERR_INIT_FT_FACE;
        goto err;
    };
    FreeType_errCode = FT_Set_Pixel_Sizes(face, 0, lineH);
    if(FreeType_errCode){
        errCode = ERR_FT_SET_PIXEL_SIZES;
        goto err;
    }

    /* Populate the new font struct with the given/default data */
    if(charW > 0)
        newFont->charW = charW;
    else
        newFont->charW = face->size->metrics.max_advance >> 6;
    newFont->lineHeight = lineH;
    newFont->charset = charset;
    newFont->tabWidth = FNT_DEFAULT_TAB_WIDTH;
    newFont->flags = flags;

    int firstVisibleChar = 0, lastVisibleChar = 0, totalVisibleChars = 0;
    switch ( charset ) {
        default: case FNT_CHARSET_ASCII:
            firstVisibleChar = FNT_ASCII_FIRST_VISIBLE_CHAR;
            lastVisibleChar = FNT_ASCII_LAST_VISIBLE_CHAR;
            totalVisibleChars = FNT_ASCII_TOTAL_VISIBLE_CHARS;
            break;
        case FNT_CHARSET_UTF8:
            // temporary
            firstVisibleChar = FNT_ASCII_FIRST_VISIBLE_CHAR;
            lastVisibleChar = FNT_ASCII_LAST_VISIBLE_CHAR;
            totalVisibleChars = FNT_ASCII_TOTAL_VISIBLE_CHARS;
            break;
    };
    newFont->visibleChars.first = firstVisibleChar;
    newFont->visibleChars.last = lastVisibleChar;
    newFont->visibleChars.total = totalVisibleChars;

    newFont->glyphs = calloc(totalVisibleChars, sizeof(fnt_GlyphData));
    if(newFont->glyphs == NULL){
        errCode = ERR_ALLOC_FNT_GLYPHS;
        goto err;
    }

    /* Prepare the temporary surfaces and the total width and height, 
     * used later to create the final surface with the appropriate dimensions */
    SDL_Surface** tmpSurfaces = malloc(sizeof(SDL_Surface*) * totalVisibleChars);
    int totalWidth = 0, totalHeight = 0;

    for(int i = 0; i < totalVisibleChars; i++){
        fnt_GlyphData *currentGlyph = &newFont->glyphs[i];

        /* char */ c = (char)(i + firstVisibleChar);
        if(FT_Load_Char(face, c, FT_LOAD_RENDER)){
            errCode = ERR_FT_LOAD_CHAR;
            goto err;
        }
        FT_GlyphSlot glyphSlot = face->glyph;

        /* Set the glyph's source rect to based on the metadata from the FT_GlyphSlot struct
         * and the current total width (which also happens to be where our 'pen' is on the X axis)
         */
        currentGlyph->srcRect = (SDL_Rect){
            .x = totalWidth,
            .y = 0,
            .w = glyphSlot->bitmap.width,
            .h = glyphSlot->bitmap.rows,
        };

        /* For some reason FreeType uses 1/64th of a pixel as its metrics unit,
         * so to obtain our wanted result we multiply everything by 64 (shift 6 bits to the right)
         */

        FT_Glyph_Metrics *m = &glyphSlot->metrics;
        if(m->horiAdvance != 0)
            currentGlyph->scaleX = (float)m->width / m->horiAdvance;

        if((float)((int)lineH << 6) != 0)
            currentGlyph->scaleY = (float)m->height / (float)((int)lineH << 6);

        if(m->horiAdvance != 0)
            currentGlyph->offsetX = (float)m->horiBearingX / m->horiAdvance;

        /* It took me 5 days to come up with this one line, 
         * so I don't think you should bother trying to understand it yourself.
         * You (probably) have the luxury to only need to know, WHAT this does, not HOW... */
        if(lineH != 0)
            currentGlyph->offsetY = 1.f - ((float)m->horiBearingY - face->descender) / (float)((int)lineH << 6);

        /* The font's texture will be a long (horizontal) line
         * with all the character placed one after the other,
         * so we increment the total width by the current glyph's width,
         * and only set the height to be the one of the taller character
         */
        totalWidth += currentGlyph->srcRect.w;
        totalHeight = max(totalHeight, glyphSlot->metrics.height >> 6);

        tmpSurfaces[i] = SDL_CreateRGBSurfaceWithFormat(
                0, glyphSlot->bitmap.width, glyphSlot->bitmap.rows, 32, SDL_PIXELFORMAT_RGBA32
            );
        SDL_Surface *currentSurface = tmpSurfaces[i];
        if(currentSurface == NULL){
            errCode = ERR_CREATE_CHAR_SURFACE;
            goto err;
        }

        /* Fill the surface with a transparent black (RGBA 0 0 0 0) background */
        SDL_LockSurface(currentSurface);
        SDL_FillRect(currentSurface, NULL, SDL_MapRGBA(currentSurface->format, 0, 0, 0, 0));

        /* Go through all the pixels one by one, 
         * and map them over onto the current glyph's temp surface */
        for (int y = 0; y < glyphSlot->bitmap.rows; ++y) {
            for (int x = 0; x < glyphSlot->bitmap.width; ++x) {
                Uint8 pixel = glyphSlot->bitmap.buffer[y * glyphSlot->bitmap.width + x];
                Uint32* target_pixel = (Uint32*)((Uint8*)currentSurface->pixels + y * currentSurface->pitch + x * sizeof(Uint32));
                *target_pixel = SDL_MapRGBA(currentSurface->format, pixel, pixel, pixel, pixel);
            }
        }
        SDL_UnlockSurface(currentSurface);
    }

    FT_Done_Face(face);
    FT_Done_FreeType(ft);

    SDL_Surface *finalSurface = SDL_CreateRGBSurfaceWithFormat(
            0, totalWidth, totalHeight, 32, SDL_PIXELFORMAT_RGBA32
        );
    if(finalSurface == NULL){
        errCode = ERR_CREATE_FINAL_SURFACE;
        goto err;
    }
    
    /* Iterate through the temporary surfaces and draw each of them, 
     * one by one, onto the final surface */
    for(int i = 0; i < totalVisibleChars; i++){
        SDL_BlitSurface(tmpSurfaces[i], NULL, finalSurface, &newFont->glyphs[i].srcRect);

        SDL_FreeSurface(tmpSurfaces[i]);
        tmpSurfaces[i] = NULL;
    }
    free(tmpSurfaces);
    tmpSurfaces = NULL;

    /* Now, create the new font's texture from the final temp surface */
    newFont->texture = SDL_CreateTextureFromSurface(renderer, finalSurface);
    if(newFont->texture == NULL){
        errCode = ERR_CREATE_FNT_TEXTURE;
        goto err;
    }
    SDL_FreeSurface(finalSurface);

    SDL_SetTextureBlendMode(newFont->texture, SDL_BLENDMODE_BLEND);
    fnt_setTextColor(newFont, 
        FNT_DEFAULT_TEXT_COLOR.r, 
        FNT_DEFAULT_TEXT_COLOR.g, 
        FNT_DEFAULT_TEXT_COLOR.b, 
        FNT_DEFAULT_TEXT_COLOR.a
    );

    return newFont;

err:
    /* Print the error message BEFORE attempting deallocation of used resources */
    switch(errCode){
        /* We don't care about 'DRY' here, totally not bc im too lazy */
        default: case EXIT_OK:
            fprintf(stderr, 
                "[fnt_initFont]: Something is bad in the code; this situation should never occur. Not freeing any resources allocated in this function!\n"
            );
            break;
    	case ERR_CREATE_FNT_TEXTURE:
	        fprintf(stderr, 
	        	"[fnt_initFont]: Failed to create the font's texture. Reason: %s.\n",
	        	 SDL_GetError()
	        );
			break;
    	case ERR_CREATE_FINAL_SURFACE:
	        fprintf(stderr, 
	        	"[fnt_initFont]: Failed to create the final surface the font's texture. Reason: %s.\n",
	        	 SDL_GetError()
	        );
			break;
    	case ERR_CREATE_CHAR_SURFACE:
	        fprintf(stderr, 
	        	"[fnt_initFont]: Failed to create a surface for the character \'%c\'. Reason: %s.\n",
	        	 c, SDL_GetError()
	        );
			break;
    	case ERR_FT_LOAD_CHAR:
	        fprintf(stderr, 
	        	"[fnt_loadFont]: Failed to load the character \'%c\'. Reason: %s",
	        	 c, FT_Error_String(FreeType_errCode)
	        );
			break;
    	case ERR_ALLOC_FNT_GLYPHS:
	        fprintf(stderr, "[fnt_loadFont]: Failed to allocate memory for the new font's glyphs.\n");
			break;
        case ERR_FT_SET_PIXEL_SIZES:
	        fprintf(stderr, 
                "[fnt_loadFont]: Failed set pixel sizes for the new font. Reason:%s.\n",
                FT_Error_String(FreeType_errCode)
            );
            break;
    	case ERR_INIT_FT_FACE:
	        fprintf(stderr, 
	        	"[fnt_loadFont]: Failed to create a FreeType face. Reason: %s.\n",
	        	 FT_Error_String(FreeType_errCode)
	        );
			break;
    	case ERR_INIT_FREETYPE:
	        fprintf(stderr, 
	        	"[fnt_loadFont]: Failed to initialize FreeType. Reason: %s.\n",
	        	 FT_Error_String(FreeType_errCode)
	        );
			break;
    	case ERR_ALLOC_FNT_STRUCT:
	        fprintf(stderr, "[fnt_loadFont]: Failed to allocate memory for the new font struct.\n");
			break;
    }
    
    /* Depending on the stage of the initialization at which the error occured,
     * free the resources that might have been allocated up to that point 
     *
     * (We are assuming tha the error codes are ordered appropriately in the EXIT_CODES enum) 
    */
    if(errCode >= ERR_CREATE_FNT_TEXTURE){
        SDL_FreeSurface(finalSurface);
        finalSurface = NULL;
    }
    if(errCode >= ERR_CREATE_FINAL_SURFACE){
        for(int i = 0; i < totalVisibleChars; i++){
            SDL_FreeSurface(tmpSurfaces[i]);
            tmpSurfaces[i] = NULL;
        }
    }
    if(errCode >= ERR_FT_LOAD_CHAR){
        free(newFont->glyphs);
        newFont->glyphs = NULL;
    }
    if(errCode >= ERR_FT_SET_PIXEL_SIZES){
        FT_Done_Face(face);
    }
    if(errCode >= ERR_INIT_FT_FACE){
        FT_Done_FreeType(ft);
    }
    if(errCode >= ERR_INIT_FREETYPE){
        free(newFont);
        newFont = NULL;
    }

    return NULL;
}

void fnt_renderText(fnt_Font *fnt, SDL_Renderer *renderer, SDL_Texture *targetTexture, fnt_Vector2D *pos, const char *fmt, ...)
{
    va_list vArgs;
    char str[FNT_TEXT_BUFFER_SIZE];

    va_start(vArgs, fmt);
    vsnprintf(str, FNT_TEXT_BUFFER_SIZE - 1, fmt, vArgs);
    va_end(vArgs);
    str[FNT_TEXT_BUFFER_SIZE - 1] = '\0';

    /* penX and penY are relative to the given baseline coorinates */
    int penX = 0, penY = 0;
    int maxPenX = 0;

    const char *c_ptr = str;
    int i;
    
    SDL_SetRenderTarget(renderer, targetTexture);
    while(*c_ptr){
        switch(*c_ptr){
            default:
                i = (int)(*c_ptr) - fnt->visibleChars.first;
                if(i < fnt->visibleChars.total && i >= 0){
                    fnt_GlyphData *currentGlyph = &fnt->glyphs[i];

                    SDL_Rect glyphDestRect = { 
                        .x = pos->x + penX + (currentGlyph->offsetX * fnt->charW),
                        .y = pos->y + penY + (currentGlyph->offsetY * fnt->lineHeight),
                        .w = (int)(fnt->charW * currentGlyph->scaleX),
                        .h = (int)(fnt->lineHeight * currentGlyph->scaleY),
                    };
                    SDL_RenderCopy(renderer, fnt->texture, &fnt->glyphs[i].srcRect, &glyphDestRect);
                    if(fnt->flags & FNT_FLAG_DISPLAY_GLYPH_RECTS){
                        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
                        SDL_RenderDrawRect(renderer, &glyphDestRect);
                    }

                    SDL_Rect charDestRect = (SDL_Rect){
                        .x = pos->x + penX, 
                        .y = pos->y + penY,
                        .w = fnt->charW,
                        .h = fnt->lineHeight
                    };
                    if(fnt->flags & FNT_FLAG_DISPLAY_CHAR_RECTS){
                        SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
                        SDL_RenderDrawRect(renderer, &charDestRect);
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
                /* Advance to the next multiple of tabWidth (times the character width) */
                if(fnt->charW != 0)
                    penX += (fnt->tabWidth -((int)(penX / fnt->charW) % fnt->tabWidth)) * fnt->charW;
                break;
            case '\n':
                maxPenX = max(penX, maxPenX);
                penX = 0;
                penY += fnt->lineHeight;
                break;
            case '\f': /* Form feed character, just treat it as two newline characters */
                penX = 0;
                penY += fnt->lineHeight * 2;
                break;
            case '\v': /* Vertical tab (newline + tab) */
                penY += fnt->lineHeight;
                maxPenX = max(penX, maxPenX);
                penX = fnt->tabWidth * fnt->charW;
                break;
        }
        c_ptr++;
    }
    if(fnt->flags & FNT_FLAG_DISPLAY_TEXT_RECTS){
        SDL_Rect textDestRect = {
            .x = pos->x,
            .y = pos->y,
            .w = max(penX, maxPenX),
            .h = penY + fnt->lineHeight,
        };
        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
        SDL_RenderDrawRect(renderer, &textDestRect);
    }

    SDL_SetRenderTarget(renderer, NULL);
}

void fnt_destroyFont(fnt_Font *fnt)
{
    free(fnt->glyphs);
    fnt->glyphs = NULL;

    SDL_DestroyTexture(fnt->texture);

    free(fnt);
    fnt = NULL;
}

void fnt_setTextColor(fnt_Font *fnt, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    SDL_SetTextureColorMod(fnt->texture, r, g, b);
    SDL_SetTextureAlphaMod(fnt->texture, a);
}
