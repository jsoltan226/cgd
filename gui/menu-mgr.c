#include "menu-mgr.h"
#include "core/datastruct/vector.h"
#include "core/log.h"
#include "core/int.h"
#include "event-listener.h"
#include "menu.h"
#include "on-event.h"
#include "input/mouse.h"
#include <SDL2/SDL_render.h>
#include <stdlib.h>
#include <error.h>

#define goto_error(msg...) do {     \
    s_log_error("menu-mgr", msg);   \
    goto err;                       \
} while (0)

struct MenuManager * menu_mgr_init(const struct menu_manager_config *cfg,
    SDL_Renderer *r, struct keyboard *keyboard, struct mouse *mouse)
{
    struct MenuManager *mmgr = calloc(1, sizeof(struct MenuManager));
    if (mmgr == NULL)
        s_log_fatal("menu-mgr", "menu_mgr_init",
            "calloc() failed for %s", "struct MenuManager");

    if (cfg->magic != MENU_CONFIG_MAGIC)
        goto_error("menu_mgr_init: The config struct is invalid");

    mmgr->full_menu_list = vector_new(struct Menu *);
    mmgr->menu_stack = vector_new(struct Menu *);
    mmgr->global_event_listeners = vector_new(struct event_listener *);

    /* Initialize the menus */
    u32 i = 0;
    while (cfg->menu_info[i].magic == MENU_CONFIG_MAGIC && i < MENUMGR_MAX_MENU_COUNT) {
        struct Menu *new_menu = menu_init(&cfg->menu_info[i], r, keyboard, mouse);
        if (new_menu == NULL)
            goto_error("menu_init for menu no. %u failed", cfg->menu_info[i].ID);

        vector_push_back(mmgr->full_menu_list, new_menu);
        i++;
    }

    /* Initialize the global event listeners */
    struct event_listener_target tmp_evl_target_obj = {
        .keyboard = keyboard,
        .mouse = mouse,
    };
    i = 0;
    while (cfg->global_event_listener_info[i].magic == MENU_CONFIG_MAGIC &&
        i < MENUMGR_MAX_MENU_COUNT) {
        struct on_event_obj tmp_onevent_obj = { 0 };
        menu_init_onevent_obj(
            &tmp_onevent_obj,
            &cfg->global_event_listener_info[i].on_event_cfg,
            NULL
        );

        struct event_listener *new_evl = event_listener_init(
            &cfg->global_event_listener_info[i].event_listener_cfg,
            &tmp_onevent_obj,
            &tmp_evl_target_obj
        );
        if (new_evl == NULL)
            goto_error("Global event_listener_init failed!");

        vector_push_back(mmgr->global_event_listeners, new_evl);
        i++;
    }

    /* Spawn into the first menu on the list */
    mmgr->curr_menu = mmgr->full_menu_list[0];

    return mmgr;

err:
    menu_mgr_destroy(mmgr);
    return NULL;
}


void menu_mgr_update(struct MenuManager *mmgr,
    struct keyboard *keyboard, struct mouse *mouse,
    bool paused)
{
    if (mmgr == NULL || keyboard == NULL || mouse == NULL) return;

    /* Update the event listeners first */
    for (u32 i = 0; i < vector_size(mmgr->global_event_listeners); i++)
        event_listener_update(mmgr->global_event_listeners[i]);

    /* If paused, only update the event listeners. */
    if(paused) return;

    /* Update the menus */
    menu_update(mmgr->curr_menu, mouse);

    /* Check the current menu's flags */
    if(mmgr->curr_menu->flags & MENU_STATUS_SWITCH &&
        mmgr->curr_menu->switch_target != MENU_ID_NULL)
    {
        mmgr->curr_menu->flags ^= MENU_STATUS_SWITCH;
        menu_mgr_push_menu(mmgr, mmgr->curr_menu->switch_target);
        mouse_force_release(mouse, MOUSE_EVERYBUTTONMASK);
    }

    if(mmgr->curr_menu->flags & MENU_STATUS_GOBACK){
        mmgr->curr_menu->flags ^= MENU_STATUS_GOBACK;
        menu_mgr_pop_menu(mmgr);
        mouse_force_release(mouse, MOUSE_EVERYBUTTONMASK);
    }
}

void menu_mgr_draw(struct MenuManager *mmgr, SDL_Renderer *r)
{
    if (mmgr == NULL || r == NULL) return;

    menu_draw(mmgr->curr_menu, r);
}

void menu_mgr_destroy(struct MenuManager *mmgr)
{
    if (mmgr == NULL) return;

    if (mmgr->full_menu_list != NULL) {
        for (u32 i = 0; i < vector_size(mmgr->full_menu_list); i++)
            menu_destroy(mmgr->full_menu_list[i]);
        vector_destroy(mmgr->full_menu_list);
    }
    vector_destroy(mmgr->menu_stack);

    if (mmgr->global_event_listeners != NULL) {
        for (u32 i = 0; i < vector_size(mmgr->global_event_listeners); i++)
            event_listener_destroy(mmgr->global_event_listeners[i]);
        vector_destroy(mmgr->global_event_listeners);
    }

    free(mmgr);
}

void menu_mgr_pop_menu(struct MenuManager *mmgr)
{
    if (mmgr == NULL || mmgr->menu_stack == NULL || mmgr->menu_stack[0] == NULL)
        return;

    mmgr->curr_menu = vector_back(mmgr->menu_stack);
}

void menu_mgr_push_menu(struct MenuManager *mmgr, u64 switch_target_ID)
{
    if (mmgr == NULL || switch_target_ID == MENU_ID_NULL) return;
    /* Check if a menu with the given ID exists in the menu list.
     * If it doesn't, don't do anything. */
    struct Menu *dest_ptr = NULL;
    for (u32 i = 0; i < vector_size(mmgr->full_menu_list); i++){
        if(mmgr->full_menu_list[i]->ID == switch_target_ID){
            dest_ptr = mmgr->full_menu_list[i];
            break;
        }
    }
    if (dest_ptr == NULL) {
        s_log_warn("menu-mgr", "Cannot switch to menu with ID %u: No such menu",
            switch_target_ID);
        return;
    }

    s_log_debug("menu-mgr", "Switching to menu with ID %lu...", dest_ptr->ID);

    /* Reset the current menu's switch_target */
    mmgr->curr_menu->switch_target = MENU_ID_NULL;

    /* Push the menu to the stack and switch to it */
    vector_push_back(mmgr->menu_stack, dest_ptr);
    mmgr->curr_menu = dest_ptr;
}
