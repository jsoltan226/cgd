#ifndef MENU_H
#define MENU_H

#include "buttons.h"
#include "event-listener.h"
#include "on-event.h"
#include "sprite.h"
#include "parallax-bg.h"
#include <cgd/input/mouse.h>
#include <SDL2/SDL_render.h>
#include <stddef.h>
#include <stdbool.h>
#include <sys/cdefs.h>

#define MN_ID_NULL  -1

typedef uint64_t mn_ID;

typedef enum {
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
} mn_OnEventType;

typedef enum {
    MN_STATUS_NONE = 0,
    MN_STATUS_GOBACK = 1,
} mn_StatusFlag;

typedef struct mn_Menu mn_Menu;

typedef struct {
    mn_OnEventType onEventType;
    union {
        struct {
            void *dest;
            void *source;
            size_t size;
        } memCopyInfo;

        mn_ID switchDestMenuID;

        mn_Menu *goBackMenuSource;

        const char *message;

        bool *boolVarPtr;

        void (*executeOther)();
    } onEventArgs;
} mn_OnEventConfig;

typedef struct parallax_bg_config mn_BGConfig;
typedef struct sprite_config mn_SpriteConfig;
typedef struct {
    struct sprite_config spriteCfg;
    mn_OnEventConfig onClickCfg;
    uint32_t flags;
} mn_ButtonConfig;
typedef struct {
    struct event_listener_config eventListenerCfg;
    mn_OnEventConfig onEventCfg;
} mn_eventListenerConfig;

struct  mn_Menu {
    struct {
        struct event_listener **ptrArray;
        int count;
    } eventListeners;

    struct {
        sprite_t **ptrArray;
        int count;
    } sprites;

    struct {
        struct button **ptrArray;
        int count;
    } buttons;

    struct parallax_bg *bg;

    mn_ID switchTo;
    mn_ID ID;
    unsigned long statusFlags;
};

typedef struct {
    mn_BGConfig bgConfig;

    struct {
        int count;
        mn_SpriteConfig *cfgs;
    } spriteInfo;

    struct {
        int count;
        mn_ButtonConfig *cfgs;
    } buttonInfo;

    struct {
        int count;
        mn_eventListenerConfig *cfgs;
    } eventListenerInfo;

    mn_ID id;
} mn_MenuConfig;

mn_Menu* mn_initMenu(mn_MenuConfig *cfg, SDL_Renderer *r,
    struct keyboard *keyboard, struct mouse *mouse);

void mn_updateMenu(mn_Menu *menu, struct mouse *mouse);
void mn_drawMenu(mn_Menu *menu, SDL_Renderer *r);
void mn_destroyMenu(mn_Menu *menu);
void mn_initOnEventObj(struct on_event_obj *on_event_obj, mn_OnEventConfig *cfg, mn_Menu *optionalMenuPtr);

#endif
