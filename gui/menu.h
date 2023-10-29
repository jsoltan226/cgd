#ifndef MENU_H
#define MENU_H

#include "buttons.h"
#include "event-listener.h"
#include "sprite.h"
#include "parallax-bg.h"
#include "../user-input/mouse.h"
#include <SDL2/SDL_render.h>
#include <stddef.h>

#define MN_ID_NULL  -1

typedef unsigned long mn_ID;

typedef enum {
    MN_ONEVENT_NONE,
    MN_ONEVENT_MEMCOPY,
    MN_ONEVENT_SWITCHMENU,
    MN_ONEVENT_GOBACK,
    MN_ONEVENT_PRINTMESSAGE,
    MN_ONEVENT_EXECUTE_OTHER,
} mn_OnEventType;

typedef enum {
    MN_STATUS_NONE = 0,
    MN_STATUS_GOBACK = 1,
} mn_StatusFlag;

typedef struct {
    mn_OnEventType onEventType;
    union {
        struct {
            void *dest;
            void *source;
            size_t size;
        } memCopy;
        const char *message;
        void (*executeOther)();
        mn_ID switchToID;
    } onEventArgs;
} mn_OnEventCfg;

typedef struct {
    evl_EventListener **eventListeners;
    int eventListenerCount;
    spr_Sprite **sprites;
    int spriteCount;
    btn_Button **buttons;
    int buttonCount;
    bg_ParallaxBG *bg;
    mn_ID switchTo;      
    mn_ID ID;
    unsigned long statusFlags;
} mn_Menu;

typedef struct {
    bg_BGConfig bgConfig;
    spr_SpriteConfig *spriteCfgs;
    int spriteCount;
    btn_SpriteConfig *buttonSpriteCfgs;
    mn_OnEventCfg *buttonOnClickCfgs;
    int buttonCount;
    evl_EventListenerConfig *eventListenerCfgs;
    mn_OnEventCfg *eventListenerOnEventConfigs;
    int eventListenerCount;
    mn_ID id;
} mn_MenuConfig;

mn_Menu* mn_initMenu(mn_MenuConfig *cfg, SDL_Renderer *r, kb_Keyboard *keyboard, ms_Mouse *mouse);
void mn_updateMenu(mn_Menu *menu, ms_Mouse *mouse);
void mn_drawMenu(mn_Menu *menu, SDL_Renderer *r, bool displayButtonHitboxes);
void mn_destroyMenu(mn_Menu *menu);

#endif
