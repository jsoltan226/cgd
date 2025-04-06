#ifndef CONFIG_H
#define CONFIG_H

#include "gui/parallax-bg.h"
#include "gui/sprite.h"
#include <core/int.h>
#include <core/shapes.h>
#include <platform/keyboard.h>
#include <platform/window.h>
#include <gui/menu.h>
#include <gui/buttons.h>
#include <gui/menu-mgr.h>
#include <gui/event-listener.h>
#include <stdbool.h>

#define LOG_FILEPATH "log.txt"

#define WINDOW_TITLE        "cgd"
#define WINDOW_W 750
#define WINDOW_H 500
#define WINDOW_RECT         (rect_t) { 0, 0, WINDOW_W, WINDOW_H }
#define WINDOW_FLAGS        (P_WINDOW_POS_CENTERED_XY | P_WINDOW_NO_ACCELERATION)

#define FPS                 60
#define FRAME_DURATION_us   (1000000 / FPS)

#if defined (CGD_GAME_CONFIG__)  || 1

#define TESTBUTTON_X        20
#define TESTBUTTON_Y        20
#define TESTBUTTON_WIDTH    200
#define TESTBUTTON_HEIGHT   200

#define TESTBUTTON2_OFFSET_X 80
#define TESTBUTTON2_OFFSET_Y 80

static bool displayButtonHitboxOutlines = false;

static const rect_t gameRect = { 20, 20, WINDOW_W - 40, WINDOW_H - 40 };

/* tb is test button */
#define tb_SrcWidth 780.f
#define tb_SrcHeight 280.f
#define tb_HitboxSrcX 0
#define tb_HitboxSrcY 121.f
#define tb_HitboxSrcWidth 770.f
#define tb_HitboxSrcHeight 160.f
#define tb_HitboxX (TESTBUTTON_X + (i32)(TESTBUTTON_WIDTH * ( tb_HitboxSrcX/tb_SrcWidth)))
#define tb_HitboxY (TESTBUTTON_Y + (i32)(TESTBUTTON_HEIGHT * ( tb_HitboxSrcY/tb_SrcHeight)))
#define tb_HitboxW (TESTBUTTON_WIDTH * (tb_HitboxSrcWidth/tb_SrcWidth))
#define tb_HitboxH (TESTBUTTON_HEIGHT * (tb_HitboxSrcHeight/tb_SrcHeight))

enum menu_IDs {
    MAIN_MENU,
    SUB_MENU
};
enum main_menu_button_IDs {
    MAIN_MENU_BUTTON_TESTBUTTON
};

static const struct parallax_bg_layer_config main_menu_parallax_bg_layers[] = {
    {
        .magic = MENU_CONFIG_MAGIC,
        .filepath = "gui/background/layer_01.png",
        .speed = 1,
    },
    {
        .magic = MENU_CONFIG_MAGIC,
        .filepath = "gui/background/layer_02.png",
        .speed = 2,
    },
    {
        .magic = MENU_CONFIG_MAGIC,
        .filepath = "gui/background/layer_03.png",
        .speed = 4,
    },
    {
        .magic = MENU_CONFIG_MAGIC,
        .filepath = "gui/background/layer_04.png",
        .speed = 16,
    },
    {
        .magic = MENU_CONFIG_MAGIC,
        .filepath = "gui/background/layer_05.png",
        .speed = 16,
    },
    { 0 }
};

static const struct menu_button_config main_menu_button_cfgs[] = {
    {
        .magic = MENU_CONFIG_MAGIC,
        .btn_cfg = {
            .sprite_cfg = {
                /* HITBOX */
                .hitbox = { tb_HitboxX, tb_HitboxY, tb_HitboxW, tb_HitboxH },

                /* SOURCE RECT */
                .src_rect = { 0, 0, tb_SrcWidth, tb_SrcHeight },

                /* DESTINATION RECT */
                .dst_rect = { TESTBUTTON_X, TESTBUTTON_Y,
                    TESTBUTTON_WIDTH, TESTBUTTON_HEIGHT },

                .texture_filepath = "gui/buttons/testButton.png"
            },
            .flags = BTN_DEFAULT_FLAGS,
        },
        .on_click_cfg = {
            .type = MENU_ONEVENT_SWITCHMENU,
            .arg = { .switch_dst_ID = SUB_MENU, },
        },
    },
    { 0 }
};
static const struct menu_button_config sub_menu_button_cfgs[] = {
    {
        .magic = MENU_CONFIG_MAGIC,
        .btn_cfg = {
            .sprite_cfg = {
                /* HITBOX */
                .hitbox = {
                    tb_HitboxX + TESTBUTTON2_OFFSET_X,
                    tb_HitboxY + TESTBUTTON2_OFFSET_Y,
                    tb_HitboxW, tb_HitboxH
                },

                /* SOURCE RECT */
                .src_rect = { 0, 0, tb_SrcWidth, tb_SrcHeight },

                /* DESTINATION RECT */
                .dst_rect = {
                    TESTBUTTON_X + TESTBUTTON2_OFFSET_X,
                    TESTBUTTON_Y + TESTBUTTON2_OFFSET_Y,
                    TESTBUTTON_WIDTH, TESTBUTTON_HEIGHT
                },

                .texture_filepath = "gui/buttons/testButton.png"
            },
            .flags = BTN_DISPLAY_HOVER_TINT | BTN_DISPLAY_HELD_TINT
                | BTN_DISPLAY_HITBOX_OUTLINE,
        },
        .on_click_cfg = {
            .type = MENU_ONEVENT_SWITCHMENU,
            .arg = { .switch_dst_ID = MAIN_MENU, },
        },
    },
    { 0 }
};

static const struct menu_event_listener_config sub_menu_listener_cfgs[] = {
    {
        .magic = MENU_CONFIG_MAGIC,
        .event_listener_cfg = {
            .type = EVL_EVENT_KEYBOARD_KEYDOWN,
            .target_info = { .keycode = KB_KEYCODE_P }
        },
        .on_event_cfg = {
            .type = MENU_ONEVENT_PRINTMESSAGE,
            .arg = { .message = "hello!\n" }
        }
    },
    { 0 }
};

static const struct menu_event_listener_config global_listener_cfgs[] = {
    {
        .magic = MENU_CONFIG_MAGIC,
        .event_listener_cfg = {
            .type = EVL_EVENT_KEYBOARD_KEYUP,
            .target_info = { .keycode = KB_KEYCODE_Q },
        },
        .on_event_cfg = {
            .type = MENU_ONEVENT_QUIT,
            .arg = { 0 },
        },
    },
    {
        .magic = MENU_CONFIG_MAGIC,
        .event_listener_cfg = {
            .type = EVL_EVENT_KEYBOARD_KEYUP,
            .target_info = { .keycode = KB_KEYCODE_ESCAPE },
        },
        .on_event_cfg = {
            .type = MENU_ONEVENT_PAUSE,
            .arg = { 0 },
        },
    },
    {
        .magic = MENU_CONFIG_MAGIC,
        .event_listener_cfg = {
            .type = EVL_EVENT_KEYBOARD_KEYDOWN,
            .target_info = { .keycode = KB_KEYCODE_H },
        },
        .on_event_cfg = {
            .type = MENU_ONEVENT_FLIPBOOL,
            .arg = { .bool_ptr = &displayButtonHitboxOutlines },
        },
    },
    { 0 }
};

static const struct menu_config menu_cfgs[] = {
    {
        .magic = MENU_CONFIG_MAGIC,
        .bg_config = {
            .magic = MENU_CONFIG_MAGIC,
            .bg_cfg = {
                .layer_cfgs = main_menu_parallax_bg_layers
            },
        },
        .sprite_info = NULL,
        .button_info = main_menu_button_cfgs,
        .event_listener_info = NULL,
        .ID = MAIN_MENU,
    },
    {
        .magic = MENU_CONFIG_MAGIC,
        .bg_config = { 0 },

        .sprite_info = NULL,

        .button_info = sub_menu_button_cfgs,
        .event_listener_info = sub_menu_listener_cfgs,
        .ID = SUB_MENU,
    },
    { 0 }
};

static const struct menu_manager_config menu_manager_cfg = {
    .magic = MENU_CONFIG_MAGIC,
    .menu_info = menu_cfgs,
    .global_event_listener_info = global_listener_cfgs,
};

#endif /* CGD_GAME_CONFIG__ */

#endif
