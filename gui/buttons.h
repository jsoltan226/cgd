#ifndef BUTTONS_H_
#define BUTTONS_H_

#include "on-event.h"
#include "sprite.h"
#include <core/int.h>
#include <render/rctx.h>
#include <platform/mouse.h>
#include <stdbool.h>

enum button_flags {
    BTN_DISPLAY_HITBOX_OUTLINE  = 1 << 0,
    BTN_DISPLAY_HOVER_TINT      = 1 << 1,
};
#define BTN_DEFAULT_FLAGS (BTN_DISPLAY_HOVER_TINT)

struct button {
    struct sprite *sprite;

    pressable_obj_t button;
    bool held; /* Whether the button was clicked and the mouse is still being held */
    bool hovering;

    struct on_event_obj on_click;

    u32 flags;
};

struct button * button_init(const struct sprite_config *sprite_cfg,
    const struct on_event_obj *on_click, u32 flags);

void button_update(struct button *btn, const struct p_mouse *mouse);

void button_draw(struct button *btn, struct r_ctx *rctx);

void button_destroy(struct button **btn_p);

#endif /* BUTTONS_H_ */
