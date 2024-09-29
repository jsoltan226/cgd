#define _GNU_SOURCE
#include <core/log.h>
#include <core/util.h>
#include <core/pixel.h>
#include <core/shapes.h>
#include <platform/mouse.h>
#include <platform/window.h>
#include <platform/event.h>
#include <render/rctx.h>
#include <render/rect.h>
#include <stdlib.h>
#include <unistd.h>

#define MODULE_NAME "mouse-test"

#define WINDOW_TITLE (const unsigned char *)MODULE_NAME
#define WINDOW_RECT (rect_t) { 0, 0, 500, 500 }
#define WINDOW_FLAGS (P_WINDOW_POS_CENTERED_XY | P_WINDOW_TYPE_AUTO)

#define MOUSE_FLAGS 0

#define FPS 60
#define RECT_W 10
#define RECT_H 10
#define RECT_COLOR (color_RGBA32_t){ 255, 50, 50, 255 }

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
    struct p_mouse_state mouse_state = { 0 };
    while (!mouse_state.buttons[P_MOUSE_BUTTON_LEFT].up) {
        /** Update section **/
        struct p_event ev;
        while (p_event_poll(&ev)) {
            if (ev.type == P_EVENT_QUIT)
                goto main_loop_breakout;
        }

        p_mouse_get_state(mouse, &mouse_state, true);

        /** Render section **/
        r_ctx_set_color(rctx, BLACK_PIXEL);
        r_reset(rctx);

        /* Draw a rectangle following the mouse */
        r_ctx_set_color(rctx, RECT_COLOR); 
        r_fill_rect(rctx, mouse_state.x, mouse_state.y, RECT_W, RECT_H);

        r_flush(rctx);

        usleep(1000000 / FPS);
    }

main_loop_breakout:

    if (mouse_state.buttons[P_MOUSE_BUTTON_LEFT].up)
        s_log_info("Pressed left mouse button. Exiting...");
    else /* if (ev.type == P_EVENT_QUIT) */
        s_log_info("Received QUIT event. Exiting...");

    /* Cleanup */
    r_ctx_destroy(rctx);
    p_mouse_destroy(mouse);
    p_window_close(win);

    return EXIT_SUCCESS;

err:
    if (rctx != NULL) r_ctx_destroy(rctx);
    if (mouse != NULL) p_mouse_destroy(mouse);
    if (win != NULL) p_window_close(win);
    return EXIT_FAILURE;
}
