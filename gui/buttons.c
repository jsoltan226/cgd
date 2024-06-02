#include "buttons.h"
#include "input/mouse.h"
#include "input/pressable-obj.h"
#include "core/util.h"
#include "core/shapes.h"
#include "core/int.h"
#include "core/function-arg-macros.h"
#include "on-event.h"
#include "sprite.h"
#include <SDL2/SDL_render.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

btn_Button *btn_initButton(spr_SpriteConfig *spriteCfg, oe_OnEvent *onClick, u32 flags, SDL_Renderer *renderer)
{
    btn_Button *btn = calloc(1, sizeof(btn_Button));
    assert(btn != NULL);

    btn->sprite = spr_initSprite(spriteCfg, renderer);
    assert(btn->sprite != NULL);

    btn->button = pressable_obj_create();
    assert(btn->button != NULL);

    btn->onClick.fn = onClick->fn;
    btn->onClick.argc = onClick->argc;
    memcpy(btn->onClick.argv, onClick->argv, OE_ARGV_SIZE * sizeof(void*));
    
    btn->flags = flags;

    return btn;
}

void btn_updateButton(btn_Button *btn, struct mouse *mouse)
{
    btn->hovering = u_collision(
        &(rect_t) { mouse->x, mouse->y, 0, 0 },
        &btn->sprite->hitbox
    );

    const pressable_obj_t *mouse_button = &mouse->buttons[MOUSE_BUTTON_LEFT];

    if (btn->is_being_clicked && (mouse_button->up || mouse_button->forceReleased)) {
        btn->is_being_clicked = false;
        if (btn->hovering) oe_executeOnEventfn(btn->onClick);
    } else if (mouse_button->down && btn->hovering) {
        btn->is_being_clicked = true;
    }

    pressable_obj_update(btn->button, btn->is_being_clicked);
}

void btn_drawButton(btn_Button *btn, SDL_Renderer *renderer)
{
    /* Draw the sprite */
    spr_drawSprite(btn->sprite, renderer);

    /* Draw the hitbox outline */
    if(btn->flags & BTN_DISPLAY_HITBOX_OUTLINE){
        static const color_RGBA32_t outline_normal = { 255, 0, 0, 255 };
        static const color_RGBA32_t outline_pressed = { 0, 255, 0, 255 };

        color_RGBA32_t outline_color = btn->is_being_clicked ? outline_pressed : outline_normal;

        SDL_SetRenderDrawColor(renderer, u_color_arg_expand(outline_color));
        SDL_RenderDrawRect(renderer, (const SDL_Rect *)&btn->sprite->hitbox);
    }
    if (btn->flags & BTN_DISPLAY_HOVER_TINT) {
        static const color_RGBA32_t hover_tint = { 30, 30, 30, 100 };
        static const color_RGBA32_t click_tint = { 20, 20, 20, 128 };

        if (btn->is_being_clicked) {
            SDL_SetRenderDrawColor(renderer, u_color_arg_expand(click_tint));
            SDL_RenderFillRect(renderer, (const SDL_Rect *)&btn->sprite->hitbox);
        } else if (btn->hovering) {
            SDL_SetRenderDrawColor(renderer, u_color_arg_expand(hover_tint));
            SDL_RenderFillRect(renderer, (const SDL_Rect *)&btn->sprite->hitbox);
        }
    }
}

void btn_destroyButton(btn_Button *btn)
{
    spr_destroySprite(btn->sprite);
    pressable_obj_destroy(btn->button);

    free(btn);
    btn = NULL;
}
