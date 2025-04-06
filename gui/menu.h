#ifndef MENU_H
#define MENU_H

#include "buttons.h"
#include "event-listener.h"
#include "on-event.h"
#include "sprite.h"
#include "parallax-bg.h"
#include <core/int.h>
#include <core/util.h>
#include <core/vector.h>
#include <render/rctx.h>
#include <platform/keyboard.h>
#include <platform/mouse.h>
#include <stddef.h>
#include <stdbool.h>

/** BASIC DEFINITIONS **/

/* The maximum amount of each type of sub-element in a menu */
#define MENU_CONFIG_MAX_LEN 128

#define MENU_ID_NULL  ((u64)-1) /* The "invalid" ID */

/* A struct representing a graphical menu -
 * the central object of the cgd GUI. */
struct Menu {
    /** "Sub-elements" of a menu **/
    VECTOR(struct event_listener *) event_listeners; /* The event listeners */
    VECTOR(struct sprite *) sprites; /* The static sprites */
    VECTOR(struct button *) buttons; /* The buttons */
    struct parallax_bg *bg; /* The background */

    u64 ID; /* A unique identifier of the menu. Must not be `MENU_ID_NULL`. */

    /* Used for temporary storage of the ID of a menu to switch to.
     * Only set while switching, otherwise should be `MENU_ID_NULL`. */
    u64 switch_target;

    /* Flags that are set when a menu is in a special state.
     * See the `enum menu_status_flag` below. */
    u64 flags;
};

/* The flags that are set when the menu is in a special state.
 * The values are pretty self-explanatory. */
enum menu_status_flag {
    MENU_STATUS_NONE        = 0,
    MENU_STATUS_SWITCH      = 1 << 0,
    MENU_STATUS_GOBACK      = 1 << 1,
};

/** MENU ONEVENT **/

/* menu onevent - An interface that defines templates for callbacks
 * that are executed when an event occurs (e.g. a button is pressed) */
enum menu_onevent_type {
    /* Do nothing. No arguments for this one. */
    MENU_ONEVENT_NONE,

    /* Switch to a menu with a given ID.
     * The only argument is the ID of the menu to be switched to. */
    MENU_ONEVENT_SWITCHMENU,

    /* Pops a menu off the stack, switching to the previous one.
     * This is just the functionality of the "Back" button in GUIs.
     * Doesn't take any arguments. */
    MENU_ONEVENT_GOBACK,

    /* Sends a global PAUSE event (see `p_event_send`).
     * Doesn't take any arguments. */
    MENU_ONEVENT_PAUSE,

    /* Sends a global QUIT event (see `p_event_send`).
     * Doesn't take any arguments. */
    MENU_ONEVENT_QUIT,

    /* Flips the value of an arbitrary `bool` variable.
     * The argument is a pointer to the `bool` to be flipped. */
    MENU_ONEVENT_FLIPBOOL,

    /* Prints an arbitrary string to the standard output.
     * The argument is the string to be printed.
     * Not really meant for use outside of testing. */
    MENU_ONEVENT_PRINTMESSAGE,

#ifndef CGD_BUILDTYPE_RELEASE
    /* Copy an arbitrary amount of data
     * from an arbitrary address to an arbitrary address.
     *
     * The argument is a `struct menu_onevent_arg_memcpy_info`
     * that contains the source and destination addresses,
     * as well as the number of bytes to be copied.
     *
     * Definetly not meant for use outside of testing.
     * Disabled in release builds entirely. */
    MENU_ONEVENT_MEMCPY,

    /* Execute an arbitrary function.
     *
     * The argument is a pointer to a function
     * that returns nothing and has no parameters (`void (*)(void)`).
     *
     * Definetly not meant for use outside of testing.
     * Disabled in release builds entirely. */
    MENU_ONEVENT_EXECUTE_OTHER,
#endif /* CGD_BUILDTYPE_RELEASE */
};

/* The struct holding the information needed to configure
 * a menu onevent object - one of the available types
 * from `enum menu_onevent_type` and arguments to the callback, if any. */
struct menu_onevent_config {
    enum menu_onevent_type type; /* The type of the callback */

    /* The possible arguments to the callback */
    union menu_onevent_arg {
        /* `MENU_ONEVENT_SWITCHMENU` - the ID of the menu to be switched to */
        u64 switch_dst_ID;

        struct Menu *go_back_dst;

        /* `MENU_ONEVENT_PRINTMESSAGE` - the message to be printed */
        const char *message;

        /* `MENU_ONEVENT_FLIPBOOL` -
         * a pointer to the `bool` variable to be flipped */
        bool *bool_ptr;

#ifndef CGD_BUILDTYPE_RELEASE
        /* `MENU_ONEVENT_MEMCPY` - the `memcpy` parameters */
        struct menu_onevent_arg_memcpy_info {
            void *dst, *src;
            u64 size;
        } memcpy_info;
        static_assert(sizeof(struct menu_onevent_arg_memcpy_info)
                        <= ONEVENT_OBJ_ARG_SIZE,
            "The struct menu_onevent_arg_memcpy_info must "
            "fit into a buffer of size ONEVENT_OBJ_ARG_SIZE");

        /* `MENU_ONEVENT_EXECUTE_OTHER` - the function pointer to be called */
        union menu_onevent_execute_other {
            void (*fn_ptr)(void);
            void *void_ptr;
        } execute_other;
#endif /* CGD_BUILDTYPE_RELEASE */
    } arg;
};

/** CONFIG-RELATED **/

/* The magic value that identifies menu config structs.
 *
 * In every config struct wrapper used by `menu_init`
 * there's a `magic` field that needs to be set to this value
 * in order for the config to not be rejected. */
#define MENU_CONFIG_MAGIC ((u8)0x40)

/* A wrapper around the basic `struct parallax_bg_config`
 * used by `menu_init`. */
struct menu_parallax_bg_config {
    u8 magic; /* Needs to be `MENU_CONFIG_MAGIC` */
    struct parallax_bg_config bg_cfg; /* The basic parallax bg config */
};

/* A wrapper around the basic `struct sprite_config`
 * used by `menu_init`. */
struct menu_sprite_config {
    u8 magic; /* Needs to be `MENU_CONFIG_MAGIC` */
    struct sprite_config sprite_cfg; /* The basic sprite config */
};

/* A wrapper around the basic `struct button_config`
 * used by `menu_init`.
 *
 * Note - when configuring a menu,
 * don't initialize the `btn_cfg`'s `on_click` member.
 * Instead, initialize the `on_click_cfg`
 * with one of the available templates (see `enum menu_onevent_type`). */
struct menu_button_config {
    u8 magic; /* Needs to be `MENU_CONFIG_MAGIC` */
    struct button_config btn_cfg;
    struct menu_onevent_config on_click_cfg;
};

/* A wrapper around the basic `struct event_listener_config`
 * used by `menu_init`.
 *
 * Note - when configuring a menu,
 * don't initialize the `event_listener_cfg`'s `on_event` member.
 * Instead, initialize the `on_event_cfg`
 * with one of the available templates (see `enum menu_onevent_type`). */
struct menu_event_listener_config {
    u8 magic;
    const struct event_listener_config event_listener_cfg;
    const struct menu_onevent_config on_event_cfg;
};

/* The struct that holds all the information needed to initialize a menu. */
struct menu_config {
    u8 magic; /* Must be `MENU_CONFIG_MAGIC` */

    /* The unique identifier of the menu. Must not be `MENU_ID_NULL`. */
    u64 ID;

    /* The configuration of the (parallax) background */
    struct menu_parallax_bg_config bg_config;

    /* An array containing all the static sprites' configuration info.
     * Must be terminated with an element initialized to `{ 0 }`.
     * Can be `NULL` if no static sprites are to be used. */
    const struct menu_sprite_config *sprite_info;

    /* An array containing all the buttons' configuration info.
     * Must be terminated with an element initialized to `{ 0 }`.
     * Can be `NULL` if no buttons are to be used. */
    const struct menu_button_config *button_info;

    /* An array containing all the event listeners' configuration info.
     * Must be terminated with an element initialized to `{ 0 }`.
     * Can be `NULL` if no event listeners are to be used. */
    const struct menu_event_listener_config *event_listener_info;
};

/** FUNCTION PROTOTYPES **/

/* Initializes a new `struct Menu` based on the configuration data in `cfg`,
 * while setting `keyboard` and `mouse` as the sources of user input. */
struct Menu * menu_init(
    const struct menu_config *cfg,
    const struct p_keyboard *keyboard,
    const struct p_mouse *mouse
);

/* Updates the `menu` with the state of the `mouse` */
void menu_update(struct Menu *menu, const struct p_mouse *mouse);

/* Draws all the relevant elements of the `menu` using the renderer `rctx`. */
void menu_draw(struct Menu *menu, struct r_ctx *rctx);

/* Destroys the menu pointed to by `menu_p`,
 * and invalidates the handle by setting `*menu_p` to `NULL`. */
void menu_destroy(struct Menu **menu_p);

#ifdef GUI_MENU_INTERNAL_GUARD__
/* Initializes a menu onevent object.
 * Exposed here only for use in `gui/menu-mgr`. */
void menu_init_onevent_obj(
    struct on_event_obj *on_event_obj,
    const struct menu_onevent_config *cfg,
    const struct Menu *optionalMenuPtr
);
#endif /* GUI_MENU_INTERNAL_GUARD__ */

#endif
