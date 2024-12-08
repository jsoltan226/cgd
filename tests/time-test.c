#include <core/int.h>
#include <core/log.h>
#include <platform/time.h>
#include <stdlib.h>

#define MODULE_NAME "time-test"
#include "log-util.h"

#define LOG_FILE "log.txt"

int cgd_main(int argc, char **argv)
{
    if (test_log_setup())
        return EXIT_FAILURE;

    timestamp_t current_UTC_time;
#define N_ITERATIONS 10
    for (u32 i = 0; i < N_ITERATIONS; i++) {
        p_time(&current_UTC_time);
        s_log_info("Current UTC time: %lli.%lli",
            current_UTC_time.s, current_UTC_time.ns);
    }

    timestamp_t start_time;
    p_time_get_ticks(&start_time);

#define SLEEP_DURATION_US 10000
    s_log_info("Sleeping for %i micro-seconds...", SLEEP_DURATION_US);
    p_time_usleep(SLEEP_DURATION_US);
    s_log_info("Micro-seconds elapsed: %lli", p_time_delta_us(&start_time));

    return EXIT_SUCCESS;
}
