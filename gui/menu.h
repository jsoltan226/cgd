#ifndef MENU_H
#define MENU_H

#include "buttons.h"
#include "event-listener.h"
#include "on-event.h"
#include "sprite.h"
#include "parallax-bg.h"
#include "input/mouse.h"
#include "core/int.h"
#include <SDL2/SDL_render.h>
#include <stddef.h>
#include <stdbool.h>

#define MENU_ID_NULL  ((u64)-1)

enum menu_onevent_type {
    MN_ONEVENT_NONE,
    MN_ONEVENT_MEMCOPY,
    MN_ONEVENT_SWITCHMENU,
    MN_ONEVENT_FLIPBOOL,
    MN_ONEVENT_PAUSE,
    MN_ONEVENT_GOBACK,
    MN_ONEVENT_PRINTMESSAGE,
    MN_ONEVENT_QUIT,
#ifndef CGD_BUILDTYPE_RELEASE
    MN_ONEVENT_EXECUTE_OTHER,
#endif /* CGD_BUILDTYPE_RELEASE */
};

enum menu_status_flag {
    MN_STATUS_NONE      = 0,
    MN_STATUS_GOBACK    = 1 << 0,
};

struct menu_onevent_config {
    enum menu_onevent_type onEventType;
    union {
        struct {
            void *dest;
            void *source;
            size_t size;
        } memCopyInfo;

        u64 switchDestMenuID;

        struct Menu *goBackMenuSource;

        const char *message;

        bool *boolVarPtr;

        void (*executeOther)();
    } onEventArgs;
};

#define menu_parallax_bg_config parallax_bg_config
#define menu_sprite_config sprite_config

struct menu_button_config {
    struct sprite_config spriteCfg;
    struct menu_onevent_config onClickCfg;
    u32 flags;
};

struct menu_event_listener_config {
    struct event_listener_config eventListenerCfg;
    struct menu_onevent_config onEventCfg;
};

struct Menu {
    struct {
        struct event_listener **ptrArray;
        u32 count;
    } eventListeners;

    struct {
        sprite_t **ptrArray;
        u32 count;
    } sprites;

    struct {
        struct button **ptrArray;
        u32 count;
    } buttons;

    struct parallax_bg *bg;

    u64 switchTo;
    u64 ID;

    u64 statusFlags;
};

struct menu_config {
    struct menu_parallax_bg_config bgConfig;

    struct {
        int count;
        struct menu_sprite_config *cfgs;
    } spriteInfo;

    struct {
        int count;
        struct menu_button_config *cfgs;
    } buttonInfo;

    struct {
        int count;
        struct menu_event_listener_config *cfgs;
    } eventListenerInfo;

    u64 id;
};

struct Menu * menu_init(struct menu_config *cfg, SDL_Renderer *r,
    struct keyboard *keyboard, struct mouse *mouse);

void menu_update(struct Menu *menu, struct mouse *mouse);
void menu_draw(struct Menu *menu, SDL_Renderer *r);
void menu_destroy(struct Menu *menu);
void menu_init_onevent_obj(struct on_event_obj *on_event_obj,
        struct menu_onevent_config *cfg, struct Menu *optionalMenuPtr);

#endif
