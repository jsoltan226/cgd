#include "buttons.h"
#include "core/log.h"
#include "input/mouse.h"
#include "core/pressable-obj.h"
#include "core/util.h"
#include "core/shapes.h"
#include "core/int.h"
#include "on-event.h"
#include "sprite.h"
#include <SDL2/SDL_render.h>
#include <stdio.h>
#include <stdlib.h>

#define MODULE_NAME "button"

struct button *button_init(const struct sprite_config *sprite_cfg,
    const struct on_event_obj *on_click,
    u32 flags, SDL_Renderer *renderer)
{
    u_check_params(sprite_cfg != NULL && on_click != NULL && renderer != NULL);

    struct button *btn = calloc(1, sizeof(struct button));
    s_assert(btn != NULL, "calloc() failed for struct button!");

    btn->sprite = sprite_init(sprite_cfg, renderer);
    if (btn->sprite == NULL) {
        s_log_error("Failed to initialize the sprite!");
        free(btn);
        return NULL;
    }

    btn->on_click.fn = on_click->fn;
    memcpy(btn->on_click.argv_buf, on_click->argv_buf, ONEVENT_OBJ_ARGV_SIZE);

    btn->flags = flags;

    return btn;
}

void button_update(struct button *btn, struct mouse *mouse)
{
    if (btn == NULL || mouse == NULL) return;

    btn->hovering = u_collision(
        &(rect_t) { mouse->x, mouse->y, 0, 0 },
        &btn->sprite->hitbox
    );

    const pressable_obj_t *mouse_button = &mouse->buttons[MOUSE_BUTTON_LEFT];

    if (btn->held && (mouse_button->up || mouse_button->force_released)) {
        btn->held = false;
        if (btn->hovering) on_event_execute(btn->on_click);
    } else if (mouse_button->down && btn->hovering) {
        btn->held = true;
    }

    pressable_obj_update(&btn->button, btn->held);
}

void button_draw(struct button *btn, SDL_Renderer *renderer)
{
    if (btn == NULL || renderer == NULL) return;

    /* Draw the sprite */
    sprite_draw(btn->sprite, renderer);

    /* Draw the hitbox outline */
    if(btn->flags & BTN_DISPLAY_HITBOX_OUTLINE){
        static const color_RGBA32_t outline_normal = { 255, 0, 0, 255 };
        static const color_RGBA32_t outline_pressed = { 0, 255, 0, 255 };

        color_RGBA32_t outline_color = btn->held ? outline_pressed : outline_normal;

        SDL_SetRenderDrawColor(renderer, u_color_arg_expand(outline_color));
        SDL_RenderDrawRect(renderer, (const SDL_Rect *)&btn->sprite->hitbox);
    }
    if (btn->flags & BTN_DISPLAY_HOVER_TINT) {
        static const color_RGBA32_t hover_tint = { 30, 30, 30, 100 };
        static const color_RGBA32_t click_tint = { 20, 20, 20, 128 };

        if (btn->held) {
            SDL_SetRenderDrawColor(renderer, u_color_arg_expand(click_tint));
            SDL_RenderFillRect(renderer, (const SDL_Rect *)&btn->sprite->hitbox);
        } else if (btn->hovering) {
            SDL_SetRenderDrawColor(renderer, u_color_arg_expand(hover_tint));
            SDL_RenderFillRect(renderer, (const SDL_Rect *)&btn->sprite->hitbox);
        }
    }
}

void button_destroy(struct button *btn)
{
    if (btn == NULL) return;

    sprite_destroy(btn->sprite);
    free(btn);
}
