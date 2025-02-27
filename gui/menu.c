#include <string.h>
#include <stdlib.h>
#include "menu.h"
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

#define MODULE_NAME "menu"

/* Here are the declarations of internal menu_onevent_api_onEvent
 * interface functions. Please see the definitions at the end of this file
 * for detailed explanations of what they do */
static i32 menu_onevent_api_memcpy(u64 argv_buf[ONEVENT_OBJ_ARGV_LEN]);
static i32 menu_onevent_api_switch_menu(u64 argv_buf[ONEVENT_OBJ_ARGV_LEN]);
static i32 menu_onevent_api_go_back(u64 argv_buf[ONEVENT_OBJ_ARGV_LEN]);
static i32 menu_onevent_api_print_message(u64 argv_buf[ONEVENT_OBJ_ARGV_LEN]);
static i32 menu_onevent_api_flip_bool(u64 argv_buf[ONEVENT_OBJ_ARGV_LEN]);
static i32 menu_onevent_api_quit(u64 argv_buf[ONEVENT_OBJ_ARGV_LEN]);
static i32 menu_onevent_api_pause(u64 argv_buf[ONEVENT_OBJ_ARGV_LEN]);
#ifndef CGD_BUILDTYPE_RELEASE
static i32 menu_onevent_api_execute_other(u64 argv_buf[ONEVENT_OBJ_ARGV_LEN]);
#endif /* CGD_BUILDTYPE_RELEASE */

struct Menu * menu_init(
    const struct menu_config *cfg,
    const struct p_keyboard *keyboard,
    const struct p_mouse *mouse
)
{
    u_check_params(cfg != NULL && keyboard != NULL && mouse != NULL);

    s_log_debug("Initializing menu with ID %lu", cfg->ID);
    struct Menu *mn = NULL;

    if (cfg->ID == MENU_ID_NULL)
        goto_error("Config struct is invalid");

    s_log_debug("(%lu) OK Verifying config struct", cfg->ID);

    mn = calloc(1, sizeof(struct Menu));
    s_assert(mn != NULL, "calloc() failed for struct menu");

    mn->sprites = vector_new(struct sprite *);
    mn->buttons = vector_new(struct button *);
    mn->event_listeners = vector_new(struct event_listener *);
    if (mn->sprites == NULL || mn->buttons == NULL || mn->event_listeners == NULL)
        goto_error("Failed to initialize member vectors");

    /* Initialize the buttons */
    u32 i = 0;
    while (cfg->button_info[i].magic == MENU_CONFIG_MAGIC && i < MENU_CONFIG_MAX_LEN) {
        struct on_event_obj tmp_onevent_obj = { 0 };
        menu_init_onevent_obj(&tmp_onevent_obj, &cfg->button_info[i].on_click_cfg, mn);

        struct button *new_btn = button_init(&cfg->button_info[i].sprite_cfg,
            &tmp_onevent_obj, cfg->button_info[i].flags);
        if (new_btn == NULL)
            goto_error("Button init failed!");

        vector_push_back(mn->buttons, new_btn);
        i++;
    }
    s_log_debug("(%lu) Initialized %u button(s)", cfg->ID, i);

    /* Initialize the event listeners */
    i = 0;
    while (cfg->event_listener_info[i].magic == MENU_CONFIG_MAGIC && i < MENU_CONFIG_MAX_LEN) {
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

        vector_push_back(mn->event_listeners, evl);
        i++;
    }
    s_log_debug("(%lu) Initialized %u event listener(s)", cfg->ID, i);

    /* Initialize the sprites */
    i = 0;
    while (cfg->sprite_info[i].magic == MENU_CONFIG_MAGIC && i < MENU_CONFIG_MAX_LEN) {
        struct sprite *s = sprite_init(&cfg->sprite_info[i].sprite_cfg);
        if (s == NULL)
            goto_error("Sprite init failed!");

        vector_push_back(mn->sprites, s);
        i++;
    }
    s_log_debug("(%lu) Initialized %u sprite(s)", cfg->ID, i);

    /* Initialize the background */
    if (cfg->bg_config.magic == MENU_CONFIG_MAGIC)
        mn->bg = parallax_bg_init(&cfg->bg_config.bg_cfg);
    s_log_debug("(%lu) Initialized the background", cfg->ID);

    mn->switch_target = MENU_ID_NULL;
    mn->ID = cfg->ID;
    mn->flags = MENU_STATUS_NONE;

    s_log_debug("OK Initalizing menu with ID %lu", cfg->ID);
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

void menu_init_onevent_obj(struct on_event_obj *oeObj,
    const struct menu_onevent_config *oeCfg, const struct Menu *menu_ptr)
{
    u_check_params(oeObj != NULL && oeCfg != NULL);

    /* =====> Do not bother reading this, see explanations in the menu.h header file    <=====
     * =====> and the below functions prefixed with `menu_onevent_api_`                 <===== */
    memset(oeObj->argv_buf, 0, ONEVENT_OBJ_ARGV_SIZE);
    switch(oeCfg->onEventType){
        default:
        case MENU_ONEVENT_NONE:
            oeObj->fn = NULL;
            break;
        case MENU_ONEVENT_SWITCHMENU:
            u_check_params(menu_ptr != NULL);
            oeObj->fn = menu_onevent_api_switch_menu;
            oeObj->argv_buf[0] = (u64)(void *)&menu_ptr->switch_target;
            oeObj->argv_buf[1] = (u64)(void *)oeCfg->onEventArgs.switch_dest_ID;
            oeObj->argv_buf[2] = (u64)(void *)&menu_ptr->flags;
            break;
        case MENU_ONEVENT_QUIT:
            oeObj->fn = menu_onevent_api_quit;
            memset(oeObj->argv_buf, 0, ONEVENT_OBJ_ARGV_SIZE);
            break;
        case MENU_ONEVENT_PAUSE:
            oeObj->fn = menu_onevent_api_pause;
            memset(oeObj->argv_buf, 0, ONEVENT_OBJ_ARGV_SIZE);
            break;
        case MENU_ONEVENT_FLIPBOOL:
            oeObj->fn = menu_onevent_api_flip_bool;
            oeObj->argv_buf[0] = (u64)(void *)oeCfg->onEventArgs.bool_ptr;
            break;
        case MENU_ONEVENT_GOBACK:
            u_check_params(menu_ptr != NULL);
            oeObj->fn = menu_onevent_api_go_back;
            oeObj->argv_buf[0] = (u64)(void *)&menu_ptr->flags;
            break;
        case MENU_ONEVENT_MEMCOPY:
            oeObj->fn = menu_onevent_api_memcpy;
            oeObj->argv_buf[0] = (u64)(void *)oeCfg->onEventArgs.memcpy_info.source;
            oeObj->argv_buf[1] = (u64)(void *)oeCfg->onEventArgs.memcpy_info.dest;
            oeObj->argv_buf[2] = (u64)(void *)oeCfg->onEventArgs.memcpy_info.size;
            break;
        case MENU_ONEVENT_PRINTMESSAGE:
            oeObj->fn = menu_onevent_api_print_message;
            oeObj->argv_buf[0] = (u64)(void *)oeCfg->onEventArgs.message;
            break;
#ifndef CGD_BUILDTYPE_RELEASE
        case MENU_ONEVENT_EXECUTE_OTHER:
            oeObj->fn = menu_onevent_api_execute_other;
            oeObj->argv_buf[0] = (u64)oeCfg->onEventArgs.execute_other.void_ptr;
            break;
#endif /* CGD_BUILDTYPE_RELEASE */
    }
}

/* =====> Here are the internal menu.c functions used for performing            <======
 * =====> the various operations selected with the menu_onevent_api interface   <====== */
static i32 menu_onevent_api_switch_menu(u64 argv_buf[ONEVENT_OBJ_ARGV_LEN])
{
    /* check for argument existence and validity */
    if(argv_buf == NULL || (void *)argv_buf[0] == NULL)
        return EXIT_FAILURE;

    /* argv[0] contains a pointer to an mn->switch_target variable,
     * which we will need to change to whatever argv[1] is */
    u64 *switch_target_ptr = (void *)argv_buf[0];

    /* argv[1] contains the direct value of the id of the menu we want to switch to */
    u64 switch_target = argv_buf[1];

    /* argv[2] contains a pointer to the flags variable of the menu,
     * which we will also need to set to trigger the switch */
    u64 *flags_ptr = (void *)argv_buf[2];

    if(switch_target_ptr == NULL || switch_target == MENU_ID_NULL || flags_ptr == NULL)
        return EXIT_FAILURE;

    *switch_target_ptr = switch_target;
    *flags_ptr |= MENU_STATUS_SWITCH;

    return EXIT_SUCCESS;
}

static i32 menu_onevent_api_go_back(u64 argv_buf[ONEVENT_OBJ_ARGV_LEN])
{
    /* check for argument existence and validity */
    if(argv_buf == NULL || (void *)argv_buf[0] == NULL)
        return EXIT_FAILURE;

    /* argv[0] contains a pointer to the flags variable of the menu,
     * which we will need to set to trigger the switch */

    u64 *flags_ptr = (void *)argv_buf[0];
    *flags_ptr |= MENU_STATUS_GOBACK;
    return EXIT_SUCCESS;
}

static i32 menu_onevent_api_memcpy(u64 argv_buf[ONEVENT_OBJ_ARGV_LEN])
{
    /* =====> AVOID USING THIS FUNCTION. IT IS NOT VERY SAFE. <===== */
    /* check for argument existence and validity */

    if(argv_buf == NULL ||
        (void *)argv_buf[0] == NULL ||
        (void *)argv_buf[1] == NULL ||
        (void *)argv_buf[2] == NULL
    ) return EXIT_FAILURE;

    const void *source = (void *)argv_buf[0];
    void *dest = (void *)argv_buf[1];
    /*
     * We are hiding a size_t direct value in argv[2], which is of type void*,
     * so we need to trick the compiler to net producing an error by first casting it to type size_t*,
     * and then dereferencing it.
    */
    size_t size = *(size_t *)argv_buf[2];
    if(memcpy(dest, source, size) == NULL)
        return EXIT_FAILURE;
    else
        return EXIT_SUCCESS;
}

static i32 menu_onevent_api_print_message(u64 argv_buf[ONEVENT_OBJ_ARGV_LEN])
{
    /* check for argument existence and validity */
    if(argv_buf == NULL || (void *)argv_buf[0] == NULL)
        return EXIT_FAILURE;

    /* here argv[0] contains the pointer to the string we want to print */
    printf("%s", (const char*)argv_buf[0]);
    return EXIT_SUCCESS;
}

static i32 menu_onevent_api_flip_bool(u64 argv_buf[ONEVENT_OBJ_ARGV_LEN])
{
    /* check for argument existence and validity */
    if(argv_buf == NULL || (void *)argv_buf[0] == NULL)
        return EXIT_FAILURE;

    /* argv[0] contains the pointer to a boolean variable,
     *so convert it to a bool* and then dereference it */
    *(bool *)argv_buf[0] = !(*(bool *)argv_buf[0]);

    return EXIT_SUCCESS;
}

static i32 menu_onevent_api_quit(u64 argv_buf[ONEVENT_OBJ_ARGV_LEN])
{
    (void) argv_buf;
    p_event_send(&(const struct p_event) { .type = P_EVENT_QUIT });
    return 0;
}

static i32 menu_onevent_api_pause(u64 argv_buf[ONEVENT_OBJ_ARGV_LEN])
{
    (void) argv_buf;
    p_event_send(&(const struct p_event) { .type = P_EVENT_PAUSE });
    return 0;
}

#ifndef CGD_BUILDTYPE_RELEASE
static i32 menu_onevent_api_execute_other(u64 argv_buf[ONEVENT_OBJ_ARGV_LEN])
{
    /* =====> THIS IS A VERY UNSAFE FUNCTION,
     * USED FOR TESTING PURPOSES ONLY!
     * DO NOT USE IT IN RELEASE CODE!!! <===== */

    /* check for argument existence and validity */
    if(argv_buf == NULL || (void *)argv_buf[0] == NULL)
        return EXIT_FAILURE;

    /* Execute the function that argv[0] points to */
    const union menu_onevent_execute_other execute_other = {
        .void_ptr = (void *)argv_buf[0]
    };
    execute_other.fn_ptr();

    return EXIT_SUCCESS;
}
#endif /* CGD_BUILDTYPE_RELEASE */
