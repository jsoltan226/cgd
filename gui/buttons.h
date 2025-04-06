#ifndef BUTTONS_H_
#define BUTTONS_H_

#include "on-event.h"
#include "sprite.h"
#include <core/int.h>
#include <render/rctx.h>
#include <platform/mouse.h>
#include <stdbool.h>

enum button_flags {
    /* A red outline around the button's hitbox will be drawn,
     * and it will turn green when the button is pressed.
     * Meant for testing purposes. */
    BTN_DISPLAY_HITBOX_OUTLINE  = 1 << 0,
#define BTN_DISPLAY_HITBOX_OUTLINE_NORMAL_COLOR \
    ((color_RGBA32_t) { 255, 0, 0, 255 }) /* Red - when not clicked */

#define BTN_DISPLAY_HITBOX_OUTLINE_CLICKED_COLOR \
    ((color_RGBA32_t) { 0, 255, 0, 255 }) /* Green - when clicked */

    /* The button will become slightly tinted when a mouse hovers over it. */
    BTN_DISPLAY_HOVER_TINT      = 1 << 1,
#define BTN_DISPLAY_HOVER_TINT_COLOR ((color_RGBA32_t) { 30, 30, 30, 100 })

    /* The button will become tinted when the mouse is held on it. */
    BTN_DISPLAY_HELD_TINT      = 1 << 2,
#define BTN_DISPLAY_HELD_TINT_COLOR ((color_RGBA32_t) { 20, 20, 20, 128 })
};
#define BTN_DEFAULT_FLAGS (BTN_DISPLAY_HOVER_TINT | BTN_DISPLAY_HELD_TINT)

/* The struct that holds information needed to create a button */
struct button_config {
    struct sprite_config sprite_cfg; /* The button's sprite */

    /* The callback that will be executed when the button is clicked */
    struct on_event_obj on_click;

    /* Additional configuration options.
     * See `enum button_flags` above. */
    u32 flags;
};

/* Button - a drawable sprite with a hitbox
 * that when clicked, executes a callback. */
struct button {
    /* The sprite that will be drawn where the button resides */
    struct sprite *sprite;

    pressable_obj_t button; /* The actual "logical" button state */

    /* Whether the button was clicked and the mouse is still being held */
    bool held;

    /* Whether a mouse is currently hovering on top of the button hitbox */
    bool hovering;

    /* The callback that will be executed when the button is clicked */
    struct on_event_obj on_click;

    /* Various additional configuration data.
     * See `enum button_flags` above. */
    u32 flags;
};

/* Creates a new button based on the configuration in `cfg`.
 * Returns `NULL` on failure. */
struct button * button_init(const struct button_config *cfg);

/* Updates `btn` based on the state of `mouse`. */
void button_update(struct button *btn, const struct p_mouse *mouse);

/* Draws the sprite of `btn` using the `rctx` renderer. */
void button_draw(struct button *btn, struct r_ctx *rctx);

/* Deallocated all resources in `*btn_p`,
 * and sets the value of `*btn_p` to `NULL`. */
void button_destroy(struct button **btn_p);

#endif /* BUTTONS_H_ */
