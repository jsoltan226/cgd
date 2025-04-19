#include <core/int.h>
#include <core/log.h>
#include <core/util.h>
#include <stdlib.h>

#define MODULE_NAME "log-test"
#include "log-util.h"

i32 cgd_main(i32 argc, char **argv)
{
    (void) argc;
    (void) argv;

    if (test_log_setup())
        return EXIT_FAILURE;

    s_log_verbose("log test start");

    const char *old_line = NULL;
    s_configure_log_line(S_LOG_VERBOSE, "%s", &old_line);

    const struct s_log_output_cfg new_output_cfg = {
        .type = S_LOG_OUTPUT_MEMORYBUF,
        .out.membuf = {
            .buf = NULL,
            .buf_size = 16,
        },
        .flag_strip_esc_sequences = true,
    };
    struct s_log_output_cfg old_output_cfg = { 0 };
    if (s_configure_log_output(S_LOG_VERBOSE, &new_output_cfg, &old_output_cfg))
        goto_error("Failed to set S_LOG_VERBOSE output to a membuf of size 16");

    s_log_verbose("1234567890abcdefghijkl");

    old_output_cfg.flag_copy = true;
    if (s_configure_log_output(S_LOG_VERBOSE, &old_output_cfg, NULL))
        goto_error("Failed to set S_LOG_VERBOSE output back to the default");

    s_configure_log_line(S_LOG_VERBOSE, old_line, NULL);

    s_log_verbose("log test end");

    return EXIT_SUCCESS;

err:
    s_log_info("Test result is FAIL");
    return EXIT_FAILURE;
}
