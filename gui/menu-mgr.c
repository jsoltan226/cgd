#define GUI_MENU_INTERNAL_GUARD__
#include "menu.h"
#undef GUI_MENU_INTERNAL_GUARD__
#include "menu-mgr.h"
#include "core/util.h"
#include "event-listener.h"
#include <core/log.h>
#include <core/int.h>
#include <core/vector.h>
#include <render/rctx.h>
#include <platform/keyboard.h>
#include <platform/mouse.h>
#include <stdlib.h>
#include <error.h>
#include <string.h>

#define MODULE_NAME "menu-mgr"

struct MenuManager * menu_mgr_init(
    const struct menu_manager_config *cfg,
    struct p_keyboard *keyboard,
    struct p_mouse *mouse
)
{
    u_check_params(cfg != NULL && keyboard != NULL && mouse != NULL);

    struct MenuManager *mmgr = calloc(1, sizeof(struct MenuManager));
    s_assert(mmgr != NULL, "calloc() failed for struct MenuManager");

    mmgr->keyboard = keyboard;
    mmgr->mouse = mouse;

    mmgr->full_menu_list = vector_new(struct Menu *);
    mmgr->menu_stack = vector_new(struct Menu *);
    mmgr->global_event_listeners = vector_new(struct event_listener *);

    if (cfg->magic != MENU_CONFIG_MAGIC)
        goto_error("%s: missing magic value in config struct", __func__);

    /* Initialize the menus */
    u32 i = 0;
    while (cfg->menu_info[i].magic == MENU_CONFIG_MAGIC &&
        i < MENUMGR_MAX_MENU_COUNT)
    {
        struct Menu *new_menu = menu_init(&cfg->menu_info[i], keyboard, mouse);
        if (new_menu == NULL) {
            goto_error("menu_init for menu no. %u failed",
                cfg->menu_info[i].ID);
        }

        vector_push_back(&mmgr->full_menu_list, new_menu);
        i++;
    }

    /* Initialize the global event listeners */
    i = 0;
    while (cfg->global_event_listener_info[i].magic == MENU_CONFIG_MAGIC &&
        i < MENUMGR_MAX_MENU_COUNT) {
        struct event_listener_config tmp_cfg = { 0 };
        memcpy(&tmp_cfg, &cfg->global_event_listener_info[i].event_listener_cfg,
            sizeof(struct event_listener_config));

        switch (cfg->global_event_listener_info[i].event_listener_cfg.type) {
            case EVL_EVENT_KEYBOARD_KEYUP:
            case EVL_EVENT_KEYBOARD_KEYDOWN:
            case EVL_EVENT_KEYBOARD_KEYPRESS:
                tmp_cfg.target_obj.keyboard_p =
                    (const struct p_keyboard **)&keyboard;
                break;
            case EVL_EVENT_MOUSE_BUTTONUP:
            case EVL_EVENT_MOUSE_BUTTONDOWN:
            case EVL_EVENT_MOUSE_BUTTONPRESS:
                tmp_cfg.target_obj.mouse_p = (const struct p_mouse **)&mouse;
                break;
            default:
                tmp_cfg.target_obj.mouse_p = NULL;
        }

        menu_init_onevent_obj(
            &tmp_cfg.on_event,
            &cfg->global_event_listener_info[i].on_event_cfg,
            NULL
        );

        struct event_listener *new_evl = event_listener_init(&tmp_cfg);
        if (new_evl == NULL)
            goto_error("Global event_listener_init failed!");

        vector_push_back(&mmgr->global_event_listeners, new_evl);
        i++;
    }

    /* Spawn into the first menu on the list */
    mmgr->curr_menu = mmgr->full_menu_list[0];

    return mmgr;

err:
    menu_mgr_destroy(&mmgr);
    return NULL;
}

void menu_mgr_update(struct MenuManager *mmgr, bool paused)
{
    if (mmgr == NULL) return;

    /* Update the event listeners first */
    for (u32 i = 0; i < vector_size(mmgr->global_event_listeners); i++)
        event_listener_update(mmgr->global_event_listeners[i]);

    /* If paused, only update the event listeners. */
    if (paused) return;

    /* Update the menus */
    menu_update(mmgr->curr_menu, mmgr->mouse);

    /* Check the current menu's flags */
    if (mmgr->curr_menu->flags & MENU_STATUS_SWITCH &&
        mmgr->curr_menu->switch_target != MENU_ID_NULL)
    {
        mmgr->curr_menu->flags ^= MENU_STATUS_SWITCH;
        menu_mgr_push_menu(mmgr, mmgr->curr_menu->switch_target);
        p_mouse_reset(mmgr->mouse, P_MOUSE_EVERYBUTTONMASK);
    }

    if (mmgr->curr_menu->flags & MENU_STATUS_GOBACK) {
        mmgr->curr_menu->flags ^= MENU_STATUS_GOBACK;
        menu_mgr_pop_menu(mmgr);
        p_mouse_reset(mmgr->mouse, P_MOUSE_EVERYBUTTONMASK);
    }
}

void menu_mgr_draw(struct MenuManager *mmgr, struct r_ctx *rctx)
{
    u_check_params(mmgr != NULL && rctx != NULL);

    menu_draw(mmgr->curr_menu, rctx);
}

void menu_mgr_destroy(struct MenuManager **mmgr_p)
{
    if (mmgr_p == NULL || *mmgr_p == NULL) return;
    struct MenuManager *mmgr = *mmgr_p;

    if (mmgr->full_menu_list != NULL) {
        for (u32 i = 0; i < vector_size(mmgr->full_menu_list); i++)
            menu_destroy(&mmgr->full_menu_list[i]);
        vector_destroy(&mmgr->full_menu_list);
    }
    vector_destroy(&mmgr->menu_stack);

    if (mmgr->global_event_listeners != NULL) {
        for (u32 i = 0; i < vector_size(mmgr->global_event_listeners); i++)
            event_listener_destroy(&mmgr->global_event_listeners[i]);
        vector_destroy(&mmgr->global_event_listeners);
    }

    u_nzfree(mmgr_p);
}

void menu_mgr_pop_menu(struct MenuManager *mmgr)
{
    u_check_params(mmgr != NULL);
    if (vector_size(mmgr->menu_stack) == 0) {
        s_log_warn("%s: empty menu stack", __func__);
        return;
    }

    mmgr->curr_menu = vector_back(mmgr->menu_stack);
}

void menu_mgr_push_menu(struct MenuManager *mmgr, u64 switch_target_ID)
{
    u_check_params(mmgr != NULL && switch_target_ID != MENU_ID_NULL);

    /* Check if a menu with the given ID exists in the menu list.
     * If it doesn't, don't do anything. */
    struct Menu *dest_ptr = NULL;
    for (u32 i = 0; i < vector_size(mmgr->full_menu_list); i++){
        if (mmgr->full_menu_list[i]->ID == switch_target_ID) {
            dest_ptr = mmgr->full_menu_list[i];
            break;
        }
    }
    if (dest_ptr == NULL) {
        s_log_warn("Cannot switch to menu with ID %lu: No such menu",
            switch_target_ID);
        return;
    }

    s_log_debug("Switching to menu with ID %lu...", dest_ptr->ID);

    /* Reset the current menu's switch_target */
    mmgr->curr_menu->switch_target = MENU_ID_NULL;

    /* Push the menu to the stack and switch to it */
    vector_push_back(&mmgr->menu_stack, dest_ptr);
    mmgr->curr_menu = dest_ptr;
}
