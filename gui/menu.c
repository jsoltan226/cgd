#include <string.h>
#include <stdlib.h>
#include "menu.h"
#include "core/log.h"
#include "event-listener.h"
#include "buttons.h"
#include "on-event.h"
#include "parallax-bg.h"
#include "sprite.h"
#include "core/datastruct/vector.h"

/* Here are the declarations of internal menu_onevent_api__onEvent interface functions. Please see the definitions at the end of this file for detailed explanations of what they do */
static i32 menu_onevent_api_memcpy(u64 argv_buf[ONEVENT_OBJ_ARGV_LEN]);
static i32 menu_onevent_api_switch_menu(u64 argv_buf[ONEVENT_OBJ_ARGV_LEN]);
static i32 menu_onevent_api_go_back(u64 argv_buf[ONEVENT_OBJ_ARGV_LEN]);
static i32 menu_onevent_api_print_message(u64 argv_buf[ONEVENT_OBJ_ARGV_LEN]);
static i32 menu_onevent_api_flip_bool(u64 argv_buf[ONEVENT_OBJ_ARGV_LEN]);
#ifndef CGD_BUILDTYPE_RELEASE
static i32 menu_onevent_api_execute_other(u64 argv_buf[ONEVENT_OBJ_ARGV_LEN]);
#endif /* CGD_BUILDTYPE_RELEASE */

#define goto_error(msg...) do { \
    s_log_error("menu", msg);   \
    goto err;                   \
} while (0)

struct Menu * menu_init(const struct menu_config *cfg, SDL_Renderer* renderer,
    struct keyboard *keyboard, struct mouse *mouse)
{
    struct Menu *mn = NULL;

    if (cfg == NULL || renderer == NULL || keyboard == NULL || mouse == NULL) {
        s_log_error("menu", "menu_init: Invalid parameters");
        return NULL;
    }

    s_log_debug("menu", "Initializing menu with ID %lu", cfg->ID);

    if (cfg->ID == MENU_ID_NULL)
        goto_error("Config struct is invalid");

    s_log_debug("menu", "(%lu) OK Verifying config struct", cfg->ID);

    mn = calloc(1, sizeof(struct Menu));
    if (mn == NULL)
        s_log_fatal("menu", "menu_init", "calloc() failed for %s", "struct Menu");

    mn->sprites = vector_new(sprite_t *);
    mn->buttons = vector_new(struct button *);
    mn->event_listeners = vector_new(struct event_listener *);
    if (mn->sprites == NULL || mn->buttons == NULL || mn->event_listeners == NULL)
        goto_error("Failed to initialize member vectors");

    /* Initialize the buttons */
    u32 i = 0;
    while (cfg->button_info[i].magic == MENU_CONFIG_MAGIC && i < MENU_CONFIG_MAX_LEN) {
        struct on_event_obj tmp_onevent_obj = { 0 };
        menu_init_onevent_obj(&tmp_onevent_obj, &cfg->button_info[i].on_click_cfg, mn);
        struct button *new_button = button_init(
            &cfg->button_info[i].sprite_cfg,
            &tmp_onevent_obj,
            cfg->button_info[i].flags,
            renderer);
        if (new_button == NULL)
            goto_error("Button init failed!");

        vector_push_back(mn->buttons, new_button);
        i++;
    }
    s_log_debug("menu", "(%lu) Initialized %u button(s)", cfg->ID, i);

    /* Initialize the event listeners */
    struct event_listener_target tmp_evl_target_obj = {
        .keyboard = keyboard,
        .mouse = mouse,
    };
    i = 0;
    while (cfg->event_listener_info[i].magic == MENU_CONFIG_MAGIC && i < MENU_CONFIG_MAX_LEN) {
        struct on_event_obj tmp_onevent_obj;
        menu_init_onevent_obj(
            &tmp_onevent_obj,
            &cfg->event_listener_info[i].on_event_cfg,
            mn
        );
        struct event_listener *evl = event_listener_init(
            &cfg->event_listener_info[i].event_listener_cfg,
            &tmp_onevent_obj,
            &tmp_evl_target_obj
        );
        if (evl == NULL)
            goto_error("Event listener init failed!");

        vector_push_back(mn->event_listeners, evl);
        i++;
    }
    s_log_debug("menu", "(%lu) Initialized %u event listener(s)", cfg->ID, i);

    /* Initialize the sprites */
    i = 0;
    while (cfg->sprite_info[i].magic == MENU_CONFIG_MAGIC && i < MENU_CONFIG_MAX_LEN) {
        sprite_t *s = sprite_init(&cfg->sprite_info[i].sprite_cfg, renderer);
        if (s == NULL)
            goto_error("Sprite init failed!");

        vector_push_back(mn->sprites, s);
        i++;
    }
    s_log_debug("menu", "(%lu) Initialized %u sprite(s)", cfg->ID, i);

    /* Initialize the background */
    if (cfg->bg_config.magic == MENU_CONFIG_MAGIC)
        mn->bg = parallax_bg_init(&cfg->bg_config.bg_cfg, renderer);
    s_log_debug("menu", "(%lu) Initialized the background", cfg->ID);

    mn->switch_target = MENU_ID_NULL;
    mn->ID = cfg->ID;
    mn->flags = MENU_STATUS_NONE;

    s_log_debug("menu", "OK Initalizing menu with ID %lu", cfg->ID);
    return mn;

err:
    menu_destroy(mn);
    return NULL;
}

void menu_update(struct Menu *mn, struct mouse *mouse)
{
    if (mn == NULL || mouse == NULL) return;

    for(u32 i = 0; i < vector_size(mn->buttons); i++)
        button_update(mn->buttons[i], mouse);

    for(u32 i = 0; i < vector_size(mn->event_listeners); i++)
        event_listener_update(mn->event_listeners[i]);

    parallax_bg_update(mn->bg);
}

void menu_draw(struct Menu *mn, SDL_Renderer *renderer)
{
    if (mn == NULL || renderer == NULL) return;

    /* Draw background below everything else */
    parallax_bg_draw(mn->bg, renderer);

    for(u32 i = 0; i < vector_size(mn->sprites); i++)
        sprite_draw(mn->sprites[i], renderer);

    for(u32 i = 0; i < vector_size(mn->buttons); i++)
        button_draw(mn->buttons[i], renderer);
}

void menu_destroy(struct Menu *mn)
{
    if (mn == NULL) return;

    if (mn->sprites != NULL) {
        for(int i = 0; i < vector_size(mn->sprites); i++)
            sprite_destroy(mn->sprites[i]);
        vector_destroy(mn->sprites);
    }

    if (mn->buttons != NULL) {
        for(int i = 0; i < vector_size(mn->buttons); i++)
            button_destroy(mn->buttons[i]);
        vector_destroy(mn->buttons);
    }

    if (mn->event_listeners != NULL) {
        for(int i = 0; i < vector_size(mn->event_listeners); i++)
            event_listener_destroy(mn->event_listeners[i]);
        vector_destroy(mn->event_listeners);
    }

    parallax_bg_destroy(mn->bg);

    free(mn);
}

void menu_init_onevent_obj(struct on_event_obj *oeObj,
    const struct menu_onevent_config *oeCfg, const struct Menu *menu_ptr)
{
    if (oeObj == NULL || oeCfg == NULL) {
        s_log_error("menu", "menu_init_onevent_obj: invalid parameters");
        return;
    }

    /* =====> Do not bother reading this, see explanations in the menu.h header file    <=====
     * =====> and the below functions prefixed with `menu_onevent_api_`                 <===== */
    memset(oeObj->argv_buf, 0, ONEVENT_OBJ_ARGV_SIZE);
    switch(oeCfg->onEventType){
        default:
        case MENU_ONEVENT_NONE:
            oeObj->fn = NULL;
            break;
        case MENU_ONEVENT_SWITCHMENU:
            if (menu_ptr == NULL) {
                s_log_error("menu", "menu_init_onevent_obj: invalid parameters");
                oeObj->fn = NULL;
                return;
            }
            oeObj->fn = &menu_onevent_api_switch_menu;
            oeObj->argv_buf[0] = (u64)(void *)&menu_ptr->switch_target;
            oeObj->argv_buf[1] = (u64)(void *)oeCfg->onEventArgs.switch_dest_ID;
            oeObj->argv_buf[2] = (u64)(void *)&menu_ptr->flags;
            break;
        case MENU_ONEVENT_QUIT: case MENU_ONEVENT_PAUSE:
        case MENU_ONEVENT_FLIPBOOL:
            oeObj->fn = &menu_onevent_api_flip_bool;
            oeObj->argv_buf[0] = (u64)(void *)oeCfg->onEventArgs.bool_ptr;
            break;
        case MENU_ONEVENT_GOBACK:
            if (menu_ptr == NULL) {
                s_log_error("menu", "menu_init_onevent_obj: invalid parameters");
                memset(oeObj, 0, sizeof(struct on_event_obj));
                return;
            }
            oeObj->fn = &menu_onevent_api_go_back;
            oeObj->argv_buf[0] = (u64)(void *)&menu_ptr->flags;
            break;
        case MENU_ONEVENT_MEMCOPY:
            oeObj->fn = &menu_onevent_api_memcpy;
            oeObj->argv_buf[0] = (u64)(void *)oeCfg->onEventArgs.memcpy_info.source;
            oeObj->argv_buf[1] = (u64)(void *)oeCfg->onEventArgs.memcpy_info.dest;
            oeObj->argv_buf[2] = (u64)(void *)oeCfg->onEventArgs.memcpy_info.size;
            break;
        case MENU_ONEVENT_PRINTMESSAGE:
            oeObj->fn = &menu_onevent_api_print_message;
            oeObj->argv_buf[0] = (u64)(void *)oeCfg->onEventArgs.message;
            break;
#ifndef CGD_BUILDTYPE_RELEASE
        case MENU_ONEVENT_EXECUTE_OTHER:
            oeObj->fn = &menu_onevent_api_execute_other;
            oeObj->argv_buf[0] = (u64)(void *)oeCfg->onEventArgs.execute_other;
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

    /* argv[0] contains the pointer to a boolean variable, so convert it to a bool* and then dereference it */
    *(bool *)argv_buf[0] = !(*(bool *)argv_buf[0]);

    return EXIT_SUCCESS;
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
    ( ( void(*)() )argv_buf[0] ) ();

    return EXIT_SUCCESS;
}
#endif /* CGD_BUILDTYPE_RELEASE */
