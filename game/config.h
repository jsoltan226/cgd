#ifndef CONFIG_H
#define CONFIG_H

#include "gui/event-listener.h"
#include "gui/menu.h"
#include "gui/menu-mgr.h"
#include "gui/buttons.h"
#include "input/keyboard.h"
#include "core/int.h"
#include "core/shapes.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_pixels.h>
#include <SDL2/SDL_rect.h>
#include <stdbool.h>

#define WINDOW_TITLE                    "cgd"
#define WINDOW_X                        SDL_WINDOWPOS_CENTERED
#define WINDOW_Y                        SDL_WINDOWPOS_CENTERED
#define WINDOW_WIDTH                    750
#define WINDOW_HEIGHT                   500
#define WINDOW_FLAGS                    SDL_WINDOW_OPENGL

#define RENDERER_FLAGS                  SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_TARGETTEXTURE
#define FRAME_DURATION                  16

#define TESTBUTTON_X                    20
#define TESTBUTTON_Y                    20
#define TESTBUTTON_WIDTH                200
#define TESTBUTTON_HEIGHT               200

static bool running = true;
static bool displayButtonHitboxOutlines = false;
static bool paused = false;

static const color_RGBA32_t rendererBg = { 30, 30, 30, 100 };
static const rect_t gameRect = { 20, 20, WINDOW_WIDTH - 40, WINDOW_HEIGHT - 40 };

/* tb is test button */
#define tb_SrcWidth 780.f
#define tb_SrcHeight 280.f
#define tb_HitboxSrcX 0
#define tb_HitboxSrcY 121.f
#define tb_HitboxSrcWidth 770.f
#define tb_HitboxSrcHeight 160.f

enum menu_IDs {
    MAIN_MENU,
    SUB_MENU
};
enum main_menu_button_IDs {
    MAIN_MENU_BUTTON_TESTBUTTON
};

static const struct menu_manager_config menu_manager_cfg = {
    .magic = MENU_CONFIG_MAGIC,
    .menu_info = {
        {
            .magic = MENU_CONFIG_MAGIC,
            .bg_config = {
                .magic = MENU_CONFIG_MAGIC,
                .bg_cfg = {
                    .magic = MENU_CONFIG_MAGIC,
                    .layer_cfgs = {
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
                    }
                },
            },
            .sprite_info = { { 0 } },
            .button_info = {
                {
                    .magic = MENU_CONFIG_MAGIC,
                    .sprite_cfg = {
                        .magic = MENU_CONFIG_MAGIC,
                        /* HITBOX */
                        .hitbox = {
                            .x = TESTBUTTON_X + (i32)( TESTBUTTON_WIDTH  * ( tb_HitboxSrcX/tb_SrcWidth  )),
                            .y = TESTBUTTON_Y + (i32)( TESTBUTTON_HEIGHT * ( tb_HitboxSrcY/tb_SrcHeight )),

                            .w = (i32)( TESTBUTTON_WIDTH  * ( tb_HitboxSrcWidth/tb_SrcWidth   )),
                            .h = (i32)( TESTBUTTON_HEIGHT * ( tb_HitboxSrcHeight/tb_SrcHeight ))
                            },

                            /* SOURCE RECT */
                            .src_rect = { 0, 0, 780, 280 },

                            /* DESTINATION RECT */
                            .dst_rect = { 20, 20, 200, 200 },

                            .texture_filepath = "gui/buttons/testButton.png"
                    },
                    .on_click_cfg = {
                        .onEventType = MENU_ONEVENT_SWITCHMENU,
                        .onEventArgs = {
                            .switch_dest_ID = SUB_MENU,
                        },
                    },
                    .flags = BTN_DEFAULT_FLAGS,
                },
                { 0 }
            },
            .event_listener_info = { { 0 } },
            .ID = MAIN_MENU,
        },
        {
            .magic = MENU_CONFIG_MAGIC,
            .bg_config = { 0 },

            .sprite_info = { { 0 } },

            .button_info = {
                {
                    .magic = MENU_CONFIG_MAGIC,
                    .sprite_cfg = {
                        .magic = MENU_CONFIG_MAGIC,
                        /* HITBOX */
                        .hitbox = {
                            .x = 100 + TESTBUTTON_X + (i32)( TESTBUTTON_WIDTH  * ( tb_HitboxSrcX/tb_SrcWidth  )),
                            .y = 150 + TESTBUTTON_Y + (i32)( TESTBUTTON_HEIGHT * ( tb_HitboxSrcY/tb_SrcHeight )),

                            .w = (i32)( TESTBUTTON_WIDTH  * ( tb_HitboxSrcWidth/tb_SrcWidth   )),
                            .h = (i32)( TESTBUTTON_HEIGHT * ( tb_HitboxSrcHeight/tb_SrcHeight ))
                        },

                        /* SOURCE RECT */
                        .src_rect = { 0, 0, 780, 280 },

                        /* DESTINATION RECT */
                        .dst_rect = { 100 + 20, 150 + 20, 200, 200 },

                        /* SDL_TEXTURE POINTER, INITIALIZED AT RUNTIME */
                        .texture_filepath = "gui/buttons/testButton.png"
                    },
                    .on_click_cfg = {
                            .onEventType = MENU_ONEVENT_SWITCHMENU,
                            .onEventArgs = { .switch_dest_ID = MAIN_MENU },
                    },
                    .flags = BTN_DISPLAY_HOVER_TINT | BTN_DISPLAY_HITBOX_OUTLINE,
                },
                { 0 }
            },

            .event_listener_info = {
                {
                    .magic = MENU_CONFIG_MAGIC,
                    .event_listener_cfg = {
                        .type = EVL_EVENT_KEYBOARD_KEYPRESS,
                        .target_info = { .keycode = KB_KEYCODE_P }
                    },
                    .on_event_cfg = {
                        .onEventType = MENU_ONEVENT_PRINTMESSAGE,
                        .onEventArgs = { .message = "hello!\n" }
                    }
                },
                { 0 }
            },

            .ID = SUB_MENU,
        },
    },
    .global_event_listener_info = {
        {
            .magic = MENU_CONFIG_MAGIC,
            .event_listener_cfg = {
                .type = EVL_EVENT_KEYBOARD_KEYUP,
                .target_info = { .keycode = KB_KEYCODE_Q },
            },
            .on_event_cfg = {
                .onEventType = MENU_ONEVENT_QUIT,
                .onEventArgs = { .bool_ptr = &running },
            },
        },
        {
            .magic = MENU_CONFIG_MAGIC,
            .event_listener_cfg = {
                .type = EVL_EVENT_KEYBOARD_KEYUP,
                .target_info = { .keycode = KB_KEYCODE_ESCAPE },
            },
            .on_event_cfg = {
                .onEventType = MENU_ONEVENT_PAUSE,
                .onEventArgs = { .bool_ptr = &paused },
            },
        },
        {
            .magic = MENU_CONFIG_MAGIC,
            .event_listener_cfg = {
                .type = EVL_EVENT_KEYBOARD_KEYDOWN,
                .target_info = { .keycode = KB_KEYCODE_H },
            },
            .on_event_cfg = {
                .onEventType = MENU_ONEVENT_FLIPBOOL,
                .onEventArgs = { .bool_ptr = &displayButtonHitboxOutlines },
            },
        },
    },
};

#endif
