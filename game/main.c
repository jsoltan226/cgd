#include "init.h"
#include "config.h"
#include "main-loop.h"
#include <core/log.h>
#include <platform/time.h>
#include <stdlib.h>
#include <stdbool.h>

#define MODULE_NAME "main"

int cgd_main(int argc, char **argv)
{
    struct main_ctx main_ctx = { 0 };
    struct gui_ctx gui_ctx = { 0 };

    if (do_early_init(argc, (const char *const *)argv, &main_ctx)) {
        s_log_error("Early init failed. Stop.");
        do_main_cleanup(&main_ctx);
        return EXIT_FAILURE;
    }

    if (do_gui_init(&gui_ctx, &main_ctx)) {
        s_log_error("GUI init failed. Stop.");
        do_gui_cleanup(&gui_ctx);
        do_main_cleanup(&main_ctx);
        return EXIT_FAILURE;
    }

    s_log_info("Init OK! Entering main loop...");
    /* MAIN LOOP */
    while (true) {
        p_time_t start_time;
        p_time(&start_time);

        process_events(&main_ctx);
        if (!main_ctx.running) break;

        update_gui(&main_ctx, &gui_ctx);
        render_gui(&main_ctx, &gui_ctx);

        i64 delta_time = p_time_delta_us(&start_time);
        if(delta_time <= FRAME_DURATION_us)
            p_time_usleep(FRAME_DURATION_us - delta_time);
    }

    s_log_debug("Exited from the main loop, starting cleanup...");
    do_gui_cleanup(&gui_ctx);
    do_main_cleanup(&main_ctx);

    s_log_info("Cleanup done, exiting with code 0 (SUCCESS).");
    return EXIT_SUCCESS;
}
