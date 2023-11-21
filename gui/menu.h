#ifndef MENU_H
#define MENU_H

#include "buttons.h"
#include "event-listener.h"
#include "sprite.h"
#include "parallax-bg.h"
#include "../user-input/mouse.h"
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
    MN_ONEVENT_EXECUTE_OTHER,
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

typedef bg_BGConfig mn_BGConfig;
typedef spr_SpriteConfig mn_SpriteConfig;
typedef struct {
    btn_SpriteConfig spriteCfg;
    mn_OnEventConfig onClickCfg;
} mn_ButtonConfig;
typedef struct {
    evl_EventListenerConfig eventListenerCfg;
    mn_OnEventConfig onEventCfg;
} mn_eventListenerConfig;

struct  mn_Menu {
    struct {
        evl_EventListener **ptrArray;
        int count;
    } eventListeners;

    struct {
        spr_Sprite **ptrArray;
        int count;
    } sprites;

    struct {
        btn_Button **ptrArray;
        int count;
    } buttons;

    bg_ParallaxBG *bg;

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

mn_Menu* mn_initMenu(mn_MenuConfig *cfg, SDL_Renderer *r, kb_Keyboard *keyboard, ms_Mouse *mouse);
void mn_updateMenu(mn_Menu *menu, ms_Mouse *mouse);
void mn_drawMenu(mn_Menu *menu, SDL_Renderer *r, bool displayButtonHitboxes);
void mn_destroyMenu(mn_Menu *menu);
void mn_initOnEventObj(oe_OnEvent *oeObj, mn_OnEventConfig *cfg, mn_Menu *optionalMenuPtr);

#endif
