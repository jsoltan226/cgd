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
#define MODULE_NAME "unknown test"
#warning MODULE_NAME not defined; defaulting to "unknown test"
#endif /* MODULE_NAME */

static FILE *log_fp = NULL;

static void test_log_cleanup(void) {
    if (log_fp != NULL) {
        fprintf(log_fp, "===== END TEST %s =====\n\n", MODULE_NAME);
        fflush(log_fp);
        fclose(log_fp);
        log_fp = NULL;
    }
    s_close_out_log_fp();
    s_close_err_log_fp();
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

    fprintf(log_fp, "===== BEGIN TEST %s =====\n", MODULE_NAME);

    s_set_log_out_filep(&log_fp);
    s_set_log_err_filep(&log_fp);
    s_set_log_level(LOG_DEBUG);

    return 0;
}

#endif /* TEST_LOG_UTIL_H_ */
