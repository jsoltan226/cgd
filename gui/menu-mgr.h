#ifndef MENUMGR_H
#define MENUMGR_H

#include "menu.h"
#include "event-listener.h"
#include <core/int.h>
#include <core/vector.h>
#include <render/rctx.h>
#include <platform/mouse.h>
#include <platform/keyboard.h>
#include <stdbool.h>

#define MENUMGR_MAX_MENU_COUNT  32

struct MenuManager {
    struct Menu *curr_menu;

    VECTOR(struct Menu *) menu_stack;

    VECTOR(struct Menu *) full_menu_list;

    VECTOR(struct event_listener *) global_event_listeners;

    struct p_keyboard *keyboard;
    struct p_mouse *mouse;
};

struct menu_manager_config {
    u8 magic;
    struct menu_config menu_info[MENUMGR_MAX_MENU_COUNT];
    struct menu_event_listener_config global_event_listener_info[MENU_CONFIG_MAX_LEN];
};

struct MenuManager * menu_mgr_init(
    const struct menu_manager_config* cfg,
    struct p_keyboard *keyboard,
    struct p_mouse *mouse
);

void menu_mgr_update(struct MenuManager *mmgr, bool paused);

void menu_mgr_draw(struct MenuManager *mmgr, struct r_ctx *rctx);

void menu_mgr_destroy(struct MenuManager **mmgr_p);

/* Used for managing the menu stack - push menu to switch to it, pop to go back */
void menu_mgr_push_menu(struct MenuManager *mmgr, u64 switch_target_ID);
void menu_mgr_pop_menu(struct MenuManager *mmgr);

#endif
