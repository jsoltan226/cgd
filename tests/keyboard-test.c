#define _GNU_SOURCE
#include <core/log.h>
#include <core/util.h>
#include <platform/window.h>
#include <platform/keyboard.h>
#include <platform/event.h>
#include <render/rctx.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define MODULE_NAME "keyboard-x11-test"

#define WINDOW_TITLE (const unsigned char *)MODULE_NAME
#define WINDOW_RECT (rect_t) { 0, 0, 500, 500 }
#define WINDOW_FLAGS (P_WINDOW_POS_CENTERED_XY | P_WINDOW_TYPE_AUTO)

#define FPS 60

static struct p_window *win = NULL;
static struct r_ctx *rctx = NULL;
static struct p_keyboard *kb = NULL;

int main(void)
{
    s_configure_log(LOG_DEBUG, stdout, stderr);

    win = p_window_open(WINDOW_TITLE, &WINDOW_RECT, WINDOW_FLAGS);
    if (win == NULL)
        goto_error("Failed to open window. Stop.");

    rctx = r_ctx_init(win, R_TYPE_SOFTWARE, 0);
    if (rctx == NULL)
        goto_error("Failed to init renderer context. Stop.");

    kb = p_keyboard_init(win);
    if (kb == NULL)
        goto_error("Failed to init keyboard. Stop.");

    struct p_event ev = { 0 };
    while (ev.type != P_EVENT_QUIT &&
        !p_keyboard_get_key(kb, KB_KEYCODE_Q)->up
    ) {
        p_event_poll(&ev);
        p_keyboard_update(kb);
        usleep(1000000 / FPS);
    }

    if (ev.type == P_EVENT_QUIT)
        s_log_debug("Received QUIT event. Exiting...");
    else
        s_log_debug("Key 'Q' Pressed. Exiting...");
    
    p_keyboard_destroy(kb);
    r_ctx_destroy(rctx);
    p_window_close(win);
    return EXIT_SUCCESS;

err:
    if (kb != NULL) p_keyboard_destroy(kb);
    if (rctx != NULL) r_ctx_destroy(rctx);
    if (win != NULL) p_window_close(win);
    return EXIT_FAILURE;
}
