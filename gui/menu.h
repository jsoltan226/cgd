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
    MN_ONCLICK_NONE,
    MN_ONCLICK_MEMCOPY,
    MN_ONCLICK_SWITCHMENU,
    MN_ONCLICK_PRINTMESSAGE,
    MN_ONCLICK_EXECUTE_OTHER,
} mn_ButtonOnClickType;

typedef struct {
    mn_ButtonOnClickType onClickType;
    union {
        struct {
            void *dest;
            void *source;
            size_t size;
        } memCopy;
        const char *message;
        void (*executeOther)();
        mn_ID switchToID;
    } onClickArgs;
} mn_ButtonOnClickCfg;

typedef struct mn_Menu mn_Menu;
struct mn_Menu {
    evl_EventListener **eventListeners;
    int eventListenerCount;
    spr_Sprite **sprites;
    int spriteCount;
    btn_Button **buttons;
    int buttonCount;
    bg_ParallaxBG *bg;
    mn_ID switchTo;      
    mn_ID ID;
};

typedef struct {
    bg_BGConfig bgConfig;
    spr_SpriteConfig *spriteCfgs;
    int spriteCount;
    btn_SpriteConfig *buttonSpriteCfgs;
    mn_ButtonOnClickCfg *buttonOnClickCfgs;
    int buttonCount;
    evl_EventListenerConfig *eventListenerCfgs;
    int eventListenerCount;
    mn_ID id;
} mn_MenuConfig;

mn_Menu* mn_initMenu(mn_MenuConfig *cfg, SDL_Renderer *r, input_Keyboard keyboard, input_Mouse *mouse);
void mn_updateMenu(mn_Menu *menu, input_Mouse *mouse);
void mn_drawMenu(mn_Menu *menu, SDL_Renderer *r, bool displayButtonHitboxes);
void mn_destroyMenu(mn_Menu *menu);

#endif
