#include <core/log.h>
#include <core/util.h>
#include <platform/ptime.h>
#include <platform/event.h>
#include <platform/window.h>
#define P_INTERNAL_GUARD__
#include <platform/keyboard.h>
#undef P_INTERNAL_GUARD__
#include <render/rctx.h>
#include <stdlib.h>
#include <string.h>

#define MODULE_NAME "keyboard-test"
#include "log-util.h"

#define WINDOW_TITLE MODULE_NAME
#define WINDOW_RECT (rect_t) { 0, 0, 500, 500 }
#define WINDOW_FLAGS (P_WINDOW_POS_CENTERED_XY | P_WINDOW_NO_ACCELERATION)

#define FPS 60

static struct p_window *win = NULL;
static struct r_ctx *rctx = NULL;
static struct p_keyboard *kb = NULL;

int cgd_main(int argc, char **argv)
{
    (void) argc;
    (void) argv;
    if (test_log_setup())
        return EXIT_FAILURE;

    win = p_window_open(WINDOW_TITLE, &WINDOW_RECT, WINDOW_FLAGS);
    if (win == NULL)
        goto_error("Failed to open window. Stop.");

    rctx = r_ctx_init(win, R_TYPE_SOFTWARE, 0);
    if (rctx == NULL)
        goto_error("Failed to init renderer context. Stop.");

    kb = p_keyboard_init(win);
    if (kb == NULL)
        goto_error("Failed to init keyboard. Stop.");

    r_reset(rctx);
    r_flush(rctx);

    struct p_event ev = { 0 };
    while (ev.type != P_EVENT_QUIT &&
        !p_keyboard_get_key(kb, KB_KEYCODE_Q)->up
    ) {
        p_event_poll(&ev);

        p_keyboard_update(kb);
        for (u32 i = 0; i < P_KEYBOARD_N_KEYS; i++) {
            if (p_keyboard_get_key(kb, i)->up)
                s_log_trace("Released key %s", p_keyboard_keycode_strings[i]);
        }

        p_time_usleep(1000000 / FPS);
    }

    if (ev.type == P_EVENT_QUIT)
        s_log_verbose("Received QUIT event. Exiting...");
    else
        s_log_verbose("Key 'Q' Pressed. Exiting...");

    p_keyboard_destroy(&kb);
    r_ctx_destroy(&rctx);
    p_window_close(&win);
    return EXIT_SUCCESS;

err:
    if (kb != NULL) p_keyboard_destroy(&kb);
    if (rctx != NULL) r_ctx_destroy(&rctx);
    if (win != NULL) p_window_close(&win);
    return EXIT_FAILURE;
}
