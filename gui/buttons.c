#include "buttons.h"
#include "user-input/mouse.h"
#include "user-input/pressable-obj.h"
#include <cgd/util/util.h>
#include <cgd/gui/on-event.h>
#include <cgd/gui/sprite.h>
#include <cgd/util/function-arg-macros.h>
#include <SDL2/SDL_render.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

btn_Button *btn_initButton(spr_SpriteConfig *spriteCfg, oe_OnEvent *onClick, uint32_t flags, SDL_Renderer *renderer)
{
    btn_Button *btn = calloc(1, sizeof(btn_Button));
    assert(btn != NULL);

    btn->sprite = spr_initSprite(spriteCfg, renderer);
    assert(btn->sprite != NULL);

    btn->button = po_createPressableObj();
    assert(btn->button != NULL);

    btn->onClick.fn = onClick->fn;
    btn->onClick.argc = onClick->argc;
    memcpy(btn->onClick.argv, onClick->argv, OE_ARGV_SIZE * sizeof(void*));
    
    btn->flags = flags;

    return btn;
}

void btn_updateButton(btn_Button *btn, ms_Mouse *mouse)
{
    btn->hovering = u_collision(
        &(SDL_Rect) { mouse->x, mouse->y, 0, 0 },
        &btn->sprite->hitbox
    );

    const po_PressableObj *mouse_button = &mouse->buttons[MS_BUTTON_LEFT];

    if (btn->is_being_clicked && (mouse_button->up || mouse_button->forceReleased)) {
        btn->is_being_clicked = false;
        if (btn->hovering) oe_executeOnEventfn(btn->onClick);
    } else if (mouse_button->down && btn->hovering) {
        btn->is_being_clicked = true;
    }

    po_updatePressableObj(btn->button, btn->is_being_clicked);
}

void btn_drawButton(btn_Button *btn, SDL_Renderer *renderer)
{
    /* Draw the sprite */
    spr_drawSprite(btn->sprite, renderer);

    /* Draw the hitbox outline */
    if(btn->flags & BTN_DISPLAY_HITBOX_OUTLINE){
        static const SDL_Color outline_normal = { 255, 0, 0, 255 };
        static const SDL_Color outline_pressed = { 0, 255, 0, 255 };

        SDL_Color outline_color = btn->is_being_clicked ? outline_pressed : outline_normal;

        SDL_SetRenderDrawColor(renderer, u_color_arg_expand(outline_color));
        SDL_RenderDrawRect(renderer, &btn->sprite->hitbox);
    }
    if (btn->flags & BTN_DISPLAY_HOVER_TINT) {
        static const SDL_Color hover_tint = { 30, 30, 30, 100 };
        static const SDL_Color click_tint = { 20, 20, 20, 128 };

        if (btn->is_being_clicked) {
            SDL_SetRenderDrawColor(renderer, u_color_arg_expand(click_tint));
            SDL_RenderFillRect(renderer, &btn->sprite->hitbox);
        } else if (btn->hovering) {
            SDL_SetRenderDrawColor(renderer, u_color_arg_expand(hover_tint));
            SDL_RenderFillRect(renderer, &btn->sprite->hitbox);
        }
    }
}

void btn_destroyButton(btn_Button *btn)
{
    spr_destroySprite(btn->sprite);
    po_destroyPressableObj(btn->button);

    free(btn);
    btn = NULL;
}
