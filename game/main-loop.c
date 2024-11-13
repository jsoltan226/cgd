#include "main-loop.h"
#include "init.h"
#include "config.h"
#include <core/shapes.h>
#include <platform/event.h>
#include <platform/mouse.h>
#include <platform/keyboard.h>
#include <render/rctx.h>
#include <render/rect.h>
#include <gui/menu-mgr.h>

void process_events(struct main_ctx *ctx)
{
    struct p_event ev;
    while(p_event_poll(&ev)) {
        if (ev.type == P_EVENT_QUIT) {
            ctx->running = false;
        } else if (ev.type == P_EVENT_PAUSE) {
            ctx->paused = !ctx->paused;
        }
    }

    p_keyboard_update(ctx->keyboard);
    p_mouse_update(ctx->mouse);
}

void update_gui(const struct main_ctx *ctx, struct gui_ctx *gui)
{
    menu_mgr_update(gui->mmgr, ctx->paused);
}

void render_gui(struct main_ctx *ctx, const struct gui_ctx *gui)
{
    r_reset(ctx->r);

    menu_mgr_draw(gui->mmgr, ctx->r);

    /* Draw the game rect */
    r_ctx_set_color(ctx->r, (color_RGBA32_t) { 200, 200, 200, 255 });
    r_draw_rect(ctx->r, rect_arg_expand(gameRect));

    r_flush(ctx->r);
}
