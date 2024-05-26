#ifndef BUTTONS_H
#define BUTTONS_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_pixels.h>
#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_render.h>
#include <cgd/user-input/mouse.h>
#include <cgd/user-input/keyboard.h>
#include <cgd/gui/on-event.h>
#include <cgd/gui/sprite.h>
#include <cgd/util/int.h>
#include <stdint.h>
#include <stdbool.h>

enum btn_flags {
    BTN_DISPLAY_HITBOX_OUTLINE  = 1 << 0,
    BTN_DISPLAY_HOVER_TINT      = 1 << 1,
};
#define BTN_DEFAULT_FLAGS (BTN_DISPLAY_HOVER_TINT)

typedef struct {
    spr_Sprite *sprite;

    po_PressableObj *button; 
    bool is_being_clicked; /* Whether the button was clicked and the mouse is still being held */
    bool hovering;

    oe_OnEvent onClick;
    
    u32 flags;
} btn_Button;

btn_Button *btn_initButton(spr_SpriteConfig *spriteCfg, oe_OnEvent *onClick, u32 flags, SDL_Renderer *renderer);
void btn_updateButton(btn_Button *btn, ms_Mouse *mouse);

void btn_drawButton(btn_Button *btn, SDL_Renderer *r);

void btn_destroyButton(btn_Button *btn);

#endif
