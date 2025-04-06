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

/* MenuManager - an object used for managing menus
 * and the interactions between them (e.g. switching menus) */
struct MenuManager {
    /* An array containing all the menus in the entire GUI */
    VECTOR(struct Menu *) full_menu_list;

    /* The menu stack (FiLo) - used for managing the switching
     * and going back between menus */
    VECTOR(struct Menu *) menu_stack;

    /* The pointer to the current menu the user interacts with */
    struct Menu *curr_menu;

    /* Global event listeners, used for managing special, global events
     * such as pause, quit, etc */
    VECTOR(struct event_listener *) global_event_listeners;

    /* The input sources that the user utilizes to interact with the menu */
    struct p_keyboard *keyboard;
    struct p_mouse *mouse;
};
#define MENUMGR_MAX_MENU_COUNT  32 /* The maximum amount of total menus */

/* The struct that holds all the information
 * required to construct a `struct MenuManager`. */
struct menu_manager_config {
    u8 magic; /* Must be `MENU_CONFIG_MAGIC` */

    /* An array containing all the menus' configuration info.
     * Must be terminated with an element initialized to `{ 0 }`.
     * Can't be `NULL`. */
    const struct menu_config *menu_info;

    /* An array containing all the global event listeners' configuration info.
     * Must be terminated with an element initialized to `{ 0 }`.
     * Can be `NULL` if no global event listeners are to be used. */
    const struct menu_event_listener_config *global_event_listener_info;
};

/* Initializes a new `struct MenuManager` based on the configuration `cfg`,
 * setting `keyboard` and `mouse` as the input sources. */
struct MenuManager * menu_mgr_init(
    const struct menu_manager_config *cfg,
    struct p_keyboard *keyboard,
    struct p_mouse *mouse
);

/* Updates the menu manager `mmgr` - polls on the global event listeners,
 * and if not `paused`, updates the current menu. */
void menu_mgr_update(struct MenuManager *mmgr, bool paused);

/* Draws the current menu of `mmgr` with the renderer `rctx`. */
void menu_mgr_draw(struct MenuManager *mmgr, struct r_ctx *rctx);

/* Destroys everything associated with `*mmgr_p`
 * (all the menus and their sub-elements),
 * and invalidates the handle by setting `*mmgr_p` to `NULL`. */
void menu_mgr_destroy(struct MenuManager **mmgr_p);

/* Used for managing the menu stack -
 * push menu to switch to it, pop to go back */
void menu_mgr_push_menu(struct MenuManager *mmgr, u64 switch_target_ID);
void menu_mgr_pop_menu(struct MenuManager *mmgr);

#endif
