#include "init.h"
#include "config.h"
#include "main-loop.h"
#include <core/log.h>
#include <platform/ptime.h>
#include <stdlib.h>
#include <stdbool.h>

#define MODULE_NAME "main"

int cgd_main(int argc, char **argv)
{
    struct platform_ctx platform_ctx = { 0 };
    struct gui_ctx gui_ctx = { 0 };

    s_log_verbose("Starting platform init...");
    if (do_platform_init(argc, (const char *const *)argv, &platform_ctx)) {
        s_log_error("Platform init failed. Stop.");
        do_platform_cleanup(&platform_ctx);
        return EXIT_FAILURE;
    }

    s_log_verbose("Starting GUI init...");
    if (do_gui_init(&gui_ctx, &platform_ctx)) {
        s_log_error("GUI init failed. Stop.");
        do_gui_cleanup(&gui_ctx);
        do_platform_cleanup(&platform_ctx);
        return EXIT_FAILURE;
    }

    s_log_info("Init OK! Entering main loop...");
    /* MAIN LOOP */
    while (true) {
        timestamp_t start_time;
        p_time_get_ticks(&start_time);

        process_events(&platform_ctx, &gui_ctx);
        if (!platform_ctx.running) break;

        update_gui(&gui_ctx);
        render_gui(&gui_ctx);

        i64 delta_time = p_time_delta_us(&start_time);
        if(delta_time <= FRAME_DURATION_us)
            p_time_usleep(FRAME_DURATION_us - delta_time);
    }

    s_log_verbose("Exited from the main loop, starting cleanup...");
    do_gui_cleanup(&gui_ctx);
    do_platform_cleanup(&platform_ctx);

    s_log_info("Cleanup done, exiting with code 0 (SUCCESS).");
    return EXIT_SUCCESS;
}
