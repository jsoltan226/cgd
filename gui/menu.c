#define GUI_MENU_INTERNAL_GUARD__
#include "menu.h"
#undef GUI_MENU_INTERNAL_GUARD__
#include "event-listener.h"
#include "buttons.h"
#include "on-event.h"
#include "parallax-bg.h"
#include "sprite.h"
#include <core/log.h>
#include <core/util.h>
#include <core/vector.h>
#include <render/rctx.h>
#include <platform/mouse.h>
#include <platform/event.h>
#include <platform/keyboard.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#define MODULE_NAME "menu"

/* Menu onevent template callbacks */
static i32 menu_onevent_api_switch_menu(const u64 arg[ONEVENT_OBJ_ARG_LEN]);
static i32 menu_onevent_api_go_back(const u64 arg[ONEVENT_OBJ_ARG_LEN]);
static i32 menu_onevent_api_pause(const u64 arg[ONEVENT_OBJ_ARG_LEN]);
static i32 menu_onevent_api_quit(const u64 arg[ONEVENT_OBJ_ARG_LEN]);
static i32 menu_onevent_api_flip_bool(const u64 arg[ONEVENT_OBJ_ARG_LEN]);
static i32 menu_onevent_api_print_message(const u64 arg[ONEVENT_OBJ_ARG_LEN]);
#ifndef CGD_BUILDTYPE_RELEASE
static i32 menu_onevent_api_memcpy(const u64 arg[ONEVENT_OBJ_ARG_LEN]);
static i32 menu_onevent_api_execute_other(const u64 arg[ONEVENT_OBJ_ARG_LEN]);
#endif /* CGD_BUILDTYPE_RELEASE */

struct Menu * menu_init(
    const struct menu_config *cfg,
    const struct p_keyboard *keyboard,
    const struct p_mouse *mouse
)
{
    u_check_params(cfg != NULL && keyboard != NULL && mouse != NULL);

    s_log_verbose("Initializing menu with ID %lu", cfg->ID);
    struct Menu *mn = NULL;

    if (cfg->ID == MENU_ID_NULL)
        goto_error("Config struct is invalid");

    s_log_trace("(%lu) OK Verifying config struct", cfg->ID);

    mn = calloc(1, sizeof(struct Menu));
    s_assert(mn != NULL, "calloc() failed for struct menu");

    mn->sprites = vector_new(struct sprite *);
    mn->buttons = vector_new(struct button *);
    mn->event_listeners = vector_new(struct event_listener *);

    /* Initialize the buttons */
    u32 i = 0;
    if (cfg->button_info != NULL) {
        while (cfg->button_info[i].magic == MENU_CONFIG_MAGIC &&
            i < MENU_CONFIG_MAX_LEN)
        {
            struct button_config tmp_cfg = cfg->button_info[i].btn_cfg;
            menu_init_onevent_obj(&tmp_cfg.on_click,
                &cfg->button_info[i].on_click_cfg, mn);

            struct button *new_btn = button_init(&tmp_cfg);
            if (new_btn == NULL)
                goto_error("Button init failed!");

            vector_push_back(&mn->buttons, new_btn);
            i++;
        }
        s_log_debug("(%lu) Initialized %u button(s)", cfg->ID, i);
    }

    /* Initialize the event listeners */
    i = 0;
    if (cfg->event_listener_info != NULL) {
        while (cfg->event_listener_info[i].magic == MENU_CONFIG_MAGIC &&
                i < MENU_CONFIG_MAX_LEN)
        {
            struct event_listener_config tmp_cfg = { 0 };
            memcpy(&tmp_cfg, &cfg->event_listener_info[i].event_listener_cfg,
                    sizeof(struct event_listener_config));

            switch(cfg->event_listener_info[i].event_listener_cfg.type) {
                case EVL_EVENT_KEYBOARD_KEYUP:
                case EVL_EVENT_KEYBOARD_KEYDOWN:
                case EVL_EVENT_KEYBOARD_KEYPRESS:
                    tmp_cfg.target_obj.keyboard_p = &keyboard;
                    break;
                case EVL_EVENT_MOUSE_BUTTONUP:
                case EVL_EVENT_MOUSE_BUTTONDOWN:
                case EVL_EVENT_MOUSE_BUTTONPRESS:
                    tmp_cfg.target_obj.mouse_p = &mouse;
                    break;
                default:
                    tmp_cfg.target_obj.mouse_p = NULL;
            }

            menu_init_onevent_obj(
                &tmp_cfg.on_event,
                &cfg->event_listener_info[i].on_event_cfg,
                mn
            );
            struct event_listener *evl = event_listener_init(&tmp_cfg);
            if (evl == NULL)
                goto_error("Event listener init failed!");

            vector_push_back(&mn->event_listeners, evl);
            i++;
        }
        s_log_debug("(%lu) Initialized %u event listener(s)", cfg->ID, i);
    }

    /* Initialize the sprites */
    i = 0;
    if (cfg->sprite_info != NULL) {
        while (cfg->sprite_info[i].magic == MENU_CONFIG_MAGIC &&
            i < MENU_CONFIG_MAX_LEN)
        {
            struct sprite *s = sprite_init(&cfg->sprite_info[i].sprite_cfg);
            if (s == NULL)
                goto_error("Sprite init failed!");

            vector_push_back(&mn->sprites, s);
            i++;
        }
    }
    s_log_debug("(%lu) Initialized %u sprite(s)", cfg->ID, i);

    /* Initialize the background */
    if (cfg->bg_config.magic == MENU_CONFIG_MAGIC) {
        mn->bg = parallax_bg_init(&cfg->bg_config.bg_cfg);
        if (mn->bg == NULL)
            goto_error("Background init failed!");

        s_log_debug("(%lu) Initialized the background", cfg->ID);
    }

    mn->switch_target = MENU_ID_NULL;
    mn->ID = cfg->ID;
    mn->flags = MENU_STATUS_NONE;

    s_log_verbose("OK Initalizing menu with ID %lu", cfg->ID);
    return mn;

err:
    menu_destroy(&mn);
    return NULL;
}

void menu_update(struct Menu *mn, const struct p_mouse *mouse)
{
    u_check_params(mn != NULL && mouse != NULL);

    for(u32 i = 0; i < vector_size(mn->buttons); i++)
        button_update(mn->buttons[i], mouse);

    for(u32 i = 0; i < vector_size(mn->event_listeners); i++)
        event_listener_update(mn->event_listeners[i]);

    if (mn->bg)
        parallax_bg_update(mn->bg);
}

void menu_draw(struct Menu *mn, struct r_ctx *rctx)
{
    u_check_params(mn != NULL && rctx != NULL);

    /* Draw background below everything else */
    parallax_bg_draw(mn->bg, rctx);

    for(u32 i = 0; i < vector_size(mn->sprites); i++)
        sprite_draw(mn->sprites[i], rctx);

    for(u32 i = 0; i < vector_size(mn->buttons); i++)
        button_draw(mn->buttons[i], rctx);
}

void menu_destroy(struct Menu **mn_p)
{
    if (mn_p == NULL || *mn_p == NULL) return;
    struct Menu *mn = *mn_p;

    if (mn->sprites != NULL) {
        for(u32 i = 0; i < vector_size(mn->sprites); i++)
            sprite_destroy(&mn->sprites[i]);
        vector_destroy(&mn->sprites);
    }

    if (mn->buttons != NULL) {
        for(u32 i = 0; i < vector_size(mn->buttons); i++)
            button_destroy(&mn->buttons[i]);
        vector_destroy(&mn->buttons);
    }

    if (mn->event_listeners != NULL) {
        for(u32 i = 0; i < vector_size(mn->event_listeners); i++)
            event_listener_destroy(&mn->event_listeners[i]);
        vector_destroy(&mn->event_listeners);
    }

    parallax_bg_destroy(&mn->bg);

    u_nzfree(mn_p);
}

void menu_init_onevent_obj(struct on_event_obj *o,
    const struct menu_onevent_config *cfg, const struct Menu *menu_ptr)
{
    u_check_params(o != NULL && cfg != NULL);

    memset(o->arg, 0, ONEVENT_OBJ_ARG_SIZE);
    switch(cfg->type){
        default:
        case MENU_ONEVENT_NONE:
            o->fn = NULL;
            break;
        case MENU_ONEVENT_SWITCHMENU:
            u_check_params(menu_ptr != NULL);
            o->fn = menu_onevent_api_switch_menu;
            o->arg[0] = (u64)(void *)&menu_ptr->switch_target;
            o->arg[1] = (u64)(void *)cfg->arg.switch_dst_ID;
            o->arg[2] = (u64)(void *)&menu_ptr->flags;
            break;
        case MENU_ONEVENT_GOBACK:
            u_check_params(menu_ptr != NULL);
            o->fn = menu_onevent_api_go_back;
            o->arg[0] = (u64 const)(void *const)&menu_ptr->flags;
            break;
        case MENU_ONEVENT_PAUSE:
            o->fn = menu_onevent_api_pause;
            break;
        case MENU_ONEVENT_QUIT:
            o->fn = menu_onevent_api_quit;
            break;
        case MENU_ONEVENT_FLIPBOOL:
            o->fn = menu_onevent_api_flip_bool;
            o->arg[0] = (u64 const)(void *const)cfg->arg.bool_ptr;
            break;
        case MENU_ONEVENT_PRINTMESSAGE:
            o->fn = menu_onevent_api_print_message;
            o->arg[0] = (u64 const)(void *const)cfg->arg.message;
            break;
#ifndef CGD_BUILDTYPE_RELEASE
        case MENU_ONEVENT_MEMCPY:
            o->fn = menu_onevent_api_memcpy;
            memcpy(o->arg, &cfg->arg.memcpy_info,
                sizeof(struct menu_onevent_arg_memcpy_info));
            break;
        case MENU_ONEVENT_EXECUTE_OTHER:
            o->fn = menu_onevent_api_execute_other;
            o->arg[0] = (u64 const)cfg->arg.execute_other.void_ptr;
            break;
#endif /* CGD_BUILDTYPE_RELEASE */
    }
}

/* Here are the internal menu.c functions used for performing
 * the various operations selected with the menu onevent api */
static i32 menu_onevent_api_switch_menu(const u64 arg[ONEVENT_OBJ_ARG_LEN])
{
    /* check for argument existence and validity */
    if (arg == NULL || (void *)arg[0] == NULL)
        return EXIT_FAILURE;

    /* argv[0] contains a pointer to an mn->switch_target variable,
     * which we will need to change to whatever argv[1] is */
    u64 *switch_target_ptr = (void *)arg[0];

    /* argv[1] contains the direct value of the id of the menu we want to switch to */
    u64 switch_target = arg[1];

    /* argv[2] contains a pointer to the flags variable of the menu,
     * which we will also need to set to trigger the switch */
    u64 *flags_ptr = (void *)arg[2];

    if (switch_target_ptr == NULL ||
        switch_target == MENU_ID_NULL ||
        flags_ptr == NULL)
    {
        return EXIT_FAILURE;
    }

    *switch_target_ptr = switch_target;
    *flags_ptr |= MENU_STATUS_SWITCH;

    return EXIT_SUCCESS;
}

static i32 menu_onevent_api_go_back(const u64 arg[ONEVENT_OBJ_ARG_LEN])
{
    /* check for argument existence and validity */
    if (arg == NULL || (void *)arg[0] == NULL)
        return EXIT_FAILURE;

    /* argv[0] contains a pointer to the flags variable of the menu,
     * which we will need to set to trigger the switch */

    u64 *flags_ptr = (void *)arg[0];
    *flags_ptr |= MENU_STATUS_GOBACK;
    return EXIT_SUCCESS;
}

static i32 menu_onevent_api_pause(const u64 arg[ONEVENT_OBJ_ARG_LEN])
{
    (void) arg;
    p_event_send(&(const struct p_event) { .type = P_EVENT_PAUSE });
    return 0;
}

static i32 menu_onevent_api_quit(const u64 arg[ONEVENT_OBJ_ARG_LEN])
{
    (void) arg;
    p_event_send(&(const struct p_event) { .type = P_EVENT_QUIT });
    return 0;
}

static i32 menu_onevent_api_flip_bool(const u64 arg[ONEVENT_OBJ_ARG_LEN])
{
    /* check for argument existence and validity */
    if (arg == NULL || (void *)arg[0] == NULL)
        return EXIT_FAILURE;

    /* argv[0] contains the pointer to a boolean variable */
    *(bool *)arg[0] = !(*(bool *)(void *)arg[0]);

    return EXIT_SUCCESS;
}

static i32 menu_onevent_api_print_message(const u64 arg[ONEVENT_OBJ_ARG_LEN])
{
#ifdef CGD_BUILDTYPE_RELEASE
    s_log_warn("%s: This function shouldn't be used outside of testing.",
        __func__);
#endif /* CGD_BUILDTYPE_RELEASE */

    /* check for argument existence and validity */
    if (arg == NULL || (void *)arg[0] == NULL)
        return EXIT_FAILURE;

    /* here argv[0] contains the pointer to the string we want to print */
    printf("%s", (const char *)(void *)arg[0]);
    return EXIT_SUCCESS;
}

#ifndef CGD_BUILDTYPE_RELEASE
static i32 menu_onevent_api_memcpy(const u64 arg[ONEVENT_OBJ_ARG_LEN])
{
    /* check for argument existence and validity */

    const void *src = (void *)arg[0];
    void *dst = (void *)arg[1];
    u64 size = arg[2];

    if (src == NULL || dst == NULL) return EXIT_FAILURE;

    memcpy(dst, src, size);
    return EXIT_SUCCESS;
}

static i32 menu_onevent_api_execute_other(const u64 arg[ONEVENT_OBJ_ARG_LEN])
{
    /* check for argument existence and validity */
    if(arg == NULL || (void *)arg[0] == NULL)
        return EXIT_FAILURE;

    /* Execute the function that argv[0] points to */
    const union menu_onevent_execute_other execute_other = {
        .void_ptr = (void *)arg[0]
    };
    execute_other.fn_ptr();

    return EXIT_SUCCESS;
}
#endif /* CGD_BUILDTYPE_RELEASE */
