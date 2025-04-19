#ifndef TEST_LOG_UTIL_H_
#define TEST_LOG_UTIL_H_

#include <core/int.h>
#include <core/log.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#ifndef TEST_LOG_FILE
#define TEST_LOG_FILE "test_log.txt"
#endif /* TEST_LOG_FILE */

#ifndef MODULE_NAME
#error MODULE_NAME not defined
#endif /* MODULE_NAME */

static FILE *log_fp = NULL;
#define MEMBUF_SIZE 4096

static void test_log_cleanup(void) {
    const char *old_line = NULL;
    s_configure_log_line(S_LOG_INFO, "%s\n\n", &old_line);
    s_log_info("===== END TEST %s =====", MODULE_NAME);
    s_configure_log_line(S_LOG_INFO, old_line, NULL);

    if (log_fp != NULL) {
        fflush(log_fp);
        fclose(log_fp);
        log_fp = NULL;
    }
}

static i32 test_log_setup(void)
{
    const char *test_log_filepath = TEST_LOG_FILE;
    if (getenv("CGD_TEST_LOG_FILE"))
        test_log_filepath = getenv("CGD_TEST_LOG_FILE");

    log_fp = fopen(test_log_filepath, "ab");
    if (log_fp == NULL) {
        fprintf(stderr, "%s: Failed to open the log file (\"%s\"): %s\n",
            __func__, TEST_LOG_FILE, strerror(errno));
        return 1;
    }

    if (atexit(test_log_cleanup)) {
        fclose(log_fp);
        log_fp = NULL;
        fprintf(stderr, "%s: Failed to atexit() the log cleanup function: %s\n",
            __func__, strerror(errno));
        return 1;
    }

    const struct s_log_output_cfg out_cfg = {
        .type = S_LOG_OUTPUT_FILE,
        .out = {
            .file = log_fp,
        },
        .flag_append = true,
        .flag_copy = true,
        .flag_strip_esc_sequences = false,
    };
    if (s_configure_log_outputs(S_LOG_ALL_MASKS, &out_cfg)) {
        fprintf(stderr, "Failed to configure log output. Stop.\n");
        return 1;
    }
    s_configure_log_level(S_LOG_TRACE);

    const char *old_line = NULL;
    s_configure_log_line(S_LOG_INFO, "%s\n", &old_line);
    s_log_info("===== BEGIN TEST %s =====", MODULE_NAME);
    s_configure_log_line(S_LOG_INFO, old_line, NULL);

    return 0;
}

#endif /* TEST_LOG_UTIL_H_ */
