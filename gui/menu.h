#ifndef MENU_H
#define MENU_H

#include "buttons.h"
#include "event-listener.h"
#include "on-event.h"
#include "sprite.h"
#include "parallax-bg.h"
#include <core/int.h>
#include <core/util.h>
#include <core/vector.h>
#include <render/rctx.h>
#include <platform/keyboard.h>
#include <platform/mouse.h>
#include <stddef.h>
#include <stdbool.h>

#define MENU_CONFIG_MAX_LEN 128
#define MENU_ID_NULL  ((u64)-1)

enum menu_onevent_type {
    MENU_ONEVENT_NONE,
    MENU_ONEVENT_MEMCOPY,
    MENU_ONEVENT_SWITCHMENU,
    MENU_ONEVENT_FLIPBOOL,
    MENU_ONEVENT_PAUSE,
    MENU_ONEVENT_GOBACK,
    MENU_ONEVENT_PRINTMESSAGE,
    MENU_ONEVENT_QUIT,
#ifndef CGD_BUILDTYPE_RELEASE
    MENU_ONEVENT_EXECUTE_OTHER,
#endif /* CGD_BUILDTYPE_RELEASE */
};

enum menu_status_flag {
    MENU_STATUS_NONE        = 0,
    MENU_STATUS_SWITCH      = 1 << 0,
    MENU_STATUS_GOBACK      = 1 << 1,
};

struct menu_onevent_config {
    enum menu_onevent_type onEventType;
    union {
        struct {
            void *dest;
            void *source;
            size_t size;
        } memcpy_info;

        u64 switch_dest_ID;

        struct Menu *go_back_dest;

        char message[u_BUF_SIZE];

        bool *bool_ptr;

#ifndef CGD_BUILDTYPE_RELEASE
        void (*execute_other)();
#endif /* CGD_BUILDTYPE_RELEASE */
    } onEventArgs;
};

#define MENU_CONFIG_MAGIC 0x40
struct menu_parallax_bg_config {
    u8 magic;
    struct parallax_bg_config bg_cfg;
};
struct menu_sprite_config {
    u8 magic;
    struct sprite_config sprite_cfg;
};
struct menu_button_config {
    u8 magic;
    struct sprite_config sprite_cfg;
    struct menu_onevent_config on_click_cfg;
    u32 flags;
};
struct menu_event_listener_config {
    u8 magic;
    struct event_listener_config event_listener_cfg;
    struct menu_onevent_config on_event_cfg;
};

struct Menu {
    VECTOR(struct event_listener *) event_listeners;
    VECTOR(sprite_t *) sprites;
    VECTOR(struct button *) buttons;

    struct parallax_bg *bg;

    u64 switch_target;
    u64 ID;

    u64 flags;
};

struct menu_config {
    u8 magic;

    struct menu_parallax_bg_config bg_config;
    struct menu_sprite_config sprite_info[MENU_CONFIG_MAX_LEN];
    struct menu_button_config button_info[MENU_CONFIG_MAX_LEN];
    struct menu_event_listener_config event_listener_info[MENU_CONFIG_MAX_LEN];

    u64 ID;
};

struct Menu * menu_init(const struct menu_config *cfg, struct r_ctx *rctx,
    struct p_keyboard *keyboard, struct p_mouse *mouse);

void menu_update(struct Menu *menu, struct p_mouse *mouse);
void menu_draw(struct Menu *menu, struct r_ctx *rctx);
void menu_destroy(struct Menu **menu_p);
void menu_init_onevent_obj(struct on_event_obj *on_event_obj,
    const struct menu_onevent_config *cfg, const struct Menu *optionalMenuPtr);

#endif
