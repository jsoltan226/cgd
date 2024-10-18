#define _GNU_SOURCE
#include <core/log.h>
#include <core/util.h>
#include <core/math.h>
#include <core/pixel.h>
#include <core/shapes.h>
#include <platform/time.h>
#include <platform/mouse.h>
#include <platform/event.h>
#include <platform/window.h>
#include <render/rctx.h>
#include <render/rect.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>

#define MODULE_NAME "mouse-test"

#define WINDOW_TITLE (const unsigned char *)MODULE_NAME
#define WINDOW_RECT (rect_t) { 0, 0, 500, 500 }
#define WINDOW_FLAGS (P_WINDOW_POS_CENTERED_XY | P_WINDOW_TYPE_AUTO)

#define MOUSE_FLAGS 0

#define FPS 60
#define FRAME_DURATION (1000000 / FPS)

#define POINTER_W 10
#define POINTER_H 10
#define POINTER_COLOR ((color_RGBA32_t) { 255, 0, 0, 255 })

#define BORDER_RECT (rect_t) { 0, 0, (WINDOW_RECT).w - 1, (WINDOW_RECT).h - 1 }
#define BORDER_COLOR WHITE_PIXEL

static struct p_window *win = NULL;
static struct r_ctx *rctx = NULL;
static struct p_mouse *mouse = NULL;

int main(void)
{
    s_configure_log(LOG_DEBUG, stdout, stderr);

    /* Initialize the window */
    win = p_window_open(WINDOW_TITLE, &WINDOW_RECT, WINDOW_FLAGS);
    if (win == NULL)
        goto_error("Failed to open the window. Stop.");

    /* Initialize the renderer */
    rctx = r_ctx_init(win, R_TYPE_SOFTWARE, 0);
    if (rctx == NULL)
        goto_error("Failed to initialize the renderer. Stop.");

    /* Initialize the mouse */
    mouse = p_mouse_init(win, MOUSE_FLAGS);
    if (mouse == NULL)
        goto_error("Failed to initialize the mouse. Stop.");

    /* Main loop */
    struct p_event ev;
    struct p_mouse_state mouse_state = { 0 };
    rect_t rect = { 0 };
    vec2d_t anchor = { 0 };
    color_RGBA32_t rect_color = { 0 };
    bool mouse_held = false;
    bool running = true;

    while (running) {
        p_time_t start_time;
        p_time(&start_time);

        while (p_event_poll(&ev)) {
            if (ev.type == P_EVENT_QUIT) {
                running = false;
                goto main_loop_breakout;
            }
        }

        p_mouse_get_state(mouse, &mouse_state, true);
        if (mouse_state.buttons[P_MOUSE_BUTTON_RIGHT].up) {
            running = false;
            break;
        }

        if (mouse_state.buttons[P_MOUSE_BUTTON_LEFT].down) {
            anchor.x = mouse_state.x;
            anchor.y = mouse_state.y;
            rect.x = mouse_state.x;
            rect.y = mouse_state.y;
            rect.w = 1;
            rect.h = 1;
            mouse_held = true;

            /* Set the rect color to a random value */
            srand(start_time.ns);
            const i32 r = rand();
            rect_color.r = (r >> 24) & 0xff;
            rect_color.g = (r >> 16) & 0xff;
            rect_color.b = (r >> 8) & 0xff;
            rect_color.a = 255;
        }
        if (mouse_state.buttons[P_MOUSE_BUTTON_LEFT].up) {
            memset(&rect, 0, sizeof(rect_t));
            mouse_held = false;
        }

        if (!mouse_state.buttons[P_MOUSE_BUTTON_LEFT].pressed)
            mouse_held = false;

        if (mouse_held) {
            rect.x = u_min(mouse_state.x, anchor.x);
            rect.y = u_min(mouse_state.y, anchor.y);
            rect.w = abs(mouse_state.x - (i32)anchor.x);
            rect.h = abs(mouse_state.y - (i32)anchor.y);
        }

        r_reset(rctx);

        if (mouse_held) {
            /* Draw the rect */
            r_ctx_set_color(rctx, rect_color);
            r_draw_rect(rctx, rect_arg_expand(rect));
        }

        /* Draw the mouse pointer */
        r_ctx_set_color(rctx, (pixel_t) { 255, 0, 0, 255 });
        r_fill_rect(rctx, mouse_state.x, mouse_state.y, POINTER_W, POINTER_H);

        /* Draw the window border */
        r_ctx_set_color(rctx, BORDER_COLOR);
        r_draw_rect(rctx, rect_arg_expand(BORDER_RECT));

        r_flush(rctx);

        i64 delta_time = p_time_delta_us(&start_time);
        if (delta_time < FRAME_DURATION)
            p_time_usleep(FRAME_DURATION - delta_time);
    }

main_loop_breakout:

    if (mouse_state.buttons[P_MOUSE_BUTTON_LEFT].up)
        s_log_info("Pressed left mouse button. Exiting...");
    else /* if (ev.type == P_EVENT_QUIT) */
        s_log_info("Received QUIT event. Exiting...");

    /* Cleanup */
    r_ctx_destroy(&rctx);
    p_mouse_destroy(&mouse);
    p_window_close(&win);

    return EXIT_SUCCESS;

err:
    if (rctx != NULL) r_ctx_destroy(&rctx);
    if (mouse != NULL) p_mouse_destroy(&mouse);
    if (win != NULL) p_window_close(&win);
    return EXIT_FAILURE;
}
