#include "buttons.h"
#include "on-event.h"
#include "sprite.h"
#include <core/log.h>
#include <core/int.h>
#include <core/util.h>
#include <core/math.h>
#include <core/shapes.h>
#include <core/pressable-obj.h>
#include <render/rctx.h>
#include <render/rect.h>
#include <render/surface.h>
#include <platform/mouse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MODULE_NAME "button"

struct button *button_init(const struct sprite_config *sprite_cfg,
    const struct on_event_obj *on_click,
    u32 flags, struct r_ctx *rctx)
{
    u_check_params(sprite_cfg != NULL && on_click != NULL && rctx != NULL);

    struct button *btn = calloc(1, sizeof(struct button));
    s_assert(btn != NULL, "calloc() failed for struct button!");

    btn->sprite = sprite_init(sprite_cfg, rctx);
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

void button_update(struct button *btn, struct p_mouse *mouse)
{
    if (btn == NULL || mouse == NULL) return;

    struct p_mouse_state mouse_state;
    p_mouse_get_state(mouse, &mouse_state, false);

    btn->hovering = u_collision(
        &(rect_t) { mouse_state.x, mouse_state.y, 0, 0 },
        &btn->sprite->hitbox
    );

    const pressable_obj_t *mouse_button = &mouse_state.buttons[P_MOUSE_BUTTON_LEFT];

    if (btn->held && (mouse_button->up || mouse_button->force_released)) {
        btn->held = false;
        if (btn->hovering) on_event_execute(btn->on_click);
    } else if (mouse_button->down && btn->hovering) {
        btn->held = true;
    }

    pressable_obj_update(&btn->button, btn->held);
}

void button_draw(struct button *btn, struct r_ctx *rctx)
{
    if (btn == NULL || rctx == NULL) return;

    /* Draw the sprite */
    sprite_draw(btn->sprite, rctx);

    /* Draw the hitbox outline */
    if(btn->flags & BTN_DISPLAY_HITBOX_OUTLINE){
        static const color_RGBA32_t outline_normal = { 255, 0, 0, 255 };
        static const color_RGBA32_t outline_pressed = { 0, 255, 0, 255 };

        color_RGBA32_t outline_color = btn->held ? outline_pressed : outline_normal;

        r_ctx_set_color(rctx, outline_color);
        r_draw_rect(rctx, rect_arg_expand(btn->sprite->hitbox));
    }
    if (btn->flags & BTN_DISPLAY_HOVER_TINT) {
        static const color_RGBA32_t hover_tint = { 30, 30, 30, 100 };
        static const color_RGBA32_t click_tint = { 20, 20, 20, 128 };

        if (btn->held) {
            r_ctx_set_color(rctx, click_tint);
            r_fill_rect(rctx, rect_arg_expand(btn->sprite->hitbox));
        } else if (btn->hovering) {
            r_ctx_set_color(rctx, hover_tint);
            r_fill_rect(rctx, rect_arg_expand(btn->sprite->hitbox));
        }
    }
}

void button_destroy(struct button **btn_p)
{
    if (btn_p == NULL || *btn_p == NULL) return;
    struct button *btn = *btn_p;

    sprite_destroy(&btn->sprite);
    u_nzfree(btn_p);
}
