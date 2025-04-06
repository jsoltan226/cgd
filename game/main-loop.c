#include "main-loop.h"
#include "init.h"
#include <core/shapes.h>
#include <platform/event.h>
#include <platform/mouse.h>
#include <platform/keyboard.h>
#include <render/rctx.h>
#include <render/rect.h>
#include <gui/menu-mgr.h>
#define CGD_GAME_CONFIG__
#include "config.h"
#undef CGD_GAME_CONFIG__

#define MODULE_NAME "main"

void process_events(struct platform_ctx *p, struct gui_ctx *gui)
{
    struct p_event ev;
    while(p_event_poll(&ev)) {
        if (ev.type == P_EVENT_QUIT) {
            p->running = false;
        } else if (ev.type == P_EVENT_PAUSE) {
            gui->paused = !gui->paused;
        }
    }

    p_keyboard_update(p->keyboard);
    p_mouse_update(p->mouse);
}

void update_gui(struct gui_ctx *gui)
{
    menu_mgr_update(gui->mmgr, gui->paused);
}

void render_gui(struct gui_ctx *gui)
{
    r_reset(gui->r);

    menu_mgr_draw(gui->mmgr, gui->r);

    /* Draw the game rect */
    r_ctx_set_color(gui->r, (color_RGBA32_t) { 200, 200, 200, 255 });
    r_draw_rect(gui->r, rect_arg_expand(gameRect));

    r_flush(gui->r);
}
