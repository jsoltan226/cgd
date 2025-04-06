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
#include <stdlib.h>
#include <string.h>

#define MODULE_NAME "button"

struct button * button_init(const struct button_config *cfg)
{
    u_check_params(cfg != NULL);

    struct button *btn = calloc(1, sizeof(struct button));
    s_assert(btn != NULL, "calloc() failed for struct button!");

    btn->sprite = sprite_init(&cfg->sprite_cfg);
    if (btn->sprite == NULL) {
        s_log_error("Failed to initialize the sprite!");
        free(btn);
        return NULL;
    }

    btn->on_click.fn = cfg->on_click.fn;
    memcpy(btn->on_click.arg, cfg->on_click.arg, ONEVENT_OBJ_ARG_SIZE);

    btn->flags = cfg->flags;

    return btn;
}

void button_update(struct button *btn, const struct p_mouse *mouse)
{
    u_check_params(btn != NULL && mouse != NULL);

    struct p_mouse_state mouse_state;
    p_mouse_get_state(mouse, &mouse_state);

    btn->hovering = u_collision(
        &(rect_t) { mouse_state.x, mouse_state.y, 0, 0 },
        &btn->sprite->hitbox
    );

    const pressable_obj_t *mouse_button =
        &mouse_state.buttons[P_MOUSE_BUTTON_LEFT];

    if (btn->held && (mouse_button->up || mouse_button->force_released)) {
        btn->held = false;

        /* Only register the click if the mouse was released
         * while inside the button hitbox */
        if (btn->hovering)
            on_event_execute(btn->on_click);
    } else if (mouse_button->down && btn->hovering) {
        btn->held = true;
    }

    pressable_obj_update(&btn->button, btn->held);
}

void button_draw(struct button *btn, struct r_ctx *rctx)
{
    u_check_params(btn != NULL && rctx != NULL);

    /* Draw the sprite below all the decorations */
    sprite_draw(btn->sprite, rctx);

    if (btn->held && btn->flags & BTN_DISPLAY_HELD_TINT) {
        r_ctx_set_color(rctx, BTN_DISPLAY_HELD_TINT_COLOR);
        r_fill_rect(rctx, rect_arg_expand(btn->sprite->hitbox));
    }
    if (btn->hovering && btn->flags & BTN_DISPLAY_HOVER_TINT) {
        r_ctx_set_color(rctx, BTN_DISPLAY_HOVER_TINT_COLOR);
        r_fill_rect(rctx, rect_arg_expand(btn->sprite->hitbox));
    }

    /* The outline should be on top of everyting */
    if (btn->flags & BTN_DISPLAY_HITBOX_OUTLINE){
        const color_RGBA32_t outline_color = btn->held ?
            BTN_DISPLAY_HITBOX_OUTLINE_CLICKED_COLOR :
            BTN_DISPLAY_HITBOX_OUTLINE_NORMAL_COLOR;

        r_ctx_set_color(rctx, outline_color);
        r_draw_rect(rctx, rect_arg_expand(btn->sprite->hitbox));
    }
}

void button_destroy(struct button **btn_p)
{
    if (btn_p == NULL || *btn_p == NULL) return;
    struct button *btn = *btn_p;

    sprite_destroy(&btn->sprite);
    u_nzfree(btn_p);
}
