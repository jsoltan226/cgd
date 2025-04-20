#include <core/int.h>
#include <core/log.h>
#include <core/util.h>
#include <core/ringbuffer.h>
#include <stdlib.h>
#include <string.h>

#define MODULE_NAME "log-test"
#include "log-util.h"

i32 cgd_main(i32 argc, char **argv)
{
    (void) argc;
    (void) argv;

    if (test_log_setup())
        return EXIT_FAILURE;

    s_log_verbose("log test start");

    /* Test the ringbuffer */
    struct ringbuffer *membuf = ringbuffer_init(S_LOG_MINIMAL_MEMBUF_SIZE);
    s_assert(membuf != NULL, "ringbuffer init failed");

#define TESTMSG_LEN (S_LOG_MINIMAL_MEMBUF_SIZE * 43 + 3)
    char msg[TESTMSG_LEN + 1] = { 0 };
    memset(msg, 'X', TESTMSG_LEN);

    const char *old_line = NULL;
    s_configure_log_line(S_LOG_VERBOSE, "%s", &old_line);

    const struct s_log_output_cfg new_output_cfg = {
        .type = S_LOG_OUTPUT_MEMORYBUF,
        .out.membuf = membuf,
        .flag_strip_esc_sequences = true,
    };
    struct s_log_output_cfg old_output_cfg = { 0 };
    if (s_configure_log_output(S_LOG_VERBOSE, &new_output_cfg, &old_output_cfg))
        goto_error("Failed to set S_LOG_VERBOSE output to a membuf of size %u",
            S_LOG_MINIMAL_MEMBUF_SIZE);

    s_log_verbose(msg);

    s_log_debug("===== BEGIN MEMBUF DUMP =====");
    for (u32 i = 0; i < S_LOG_MINIMAL_MEMBUF_SIZE; i++) {
        s_log_debug("[%#x]: %#x (%c)", i, membuf->buf[i], membuf->buf[i]);
    }
    s_log_debug("===== END MEMBUF DUMP =====");

#define TEST_STRING "TEST\n"
    s_log_verbose(TEST_STRING);

    s_log_debug("===== BEGIN MEMBUF DUMP =====");
    for (u32 i = 0; i < S_LOG_MINIMAL_MEMBUF_SIZE; i++) {
        s_log_debug("[%#x]: %#x (%c)", i, membuf->buf[i], membuf->buf[i]);
    }
    s_log_debug("===== END MEMBUF DUMP =====");

    const u64 offset = (TESTMSG_LEN % (S_LOG_MINIMAL_MEMBUF_SIZE - 1));
    if (strncmp(membuf->buf + offset, TEST_STRING, S_LOG_MINIMAL_MEMBUF_SIZE))
        goto_error("membuf wrap-around test failed");

    old_output_cfg.flag_copy = true;
    if (s_configure_log_output(S_LOG_VERBOSE, &old_output_cfg, NULL))
        goto_error("Failed to set S_LOG_VERBOSE output back to the default");

    ringbuffer_destroy(&membuf);

    s_configure_log_line(S_LOG_VERBOSE, old_line, NULL);

    s_log_verbose("log test end");

    return EXIT_SUCCESS;

err:
    if (old_line != NULL) {
        s_configure_log_line(S_LOG_VERBOSE, old_line, NULL);
    }
    if (old_output_cfg.type == S_LOG_OUTPUT_FILE) {
        old_output_cfg.flag_copy = true;
        (void) s_configure_log_output(S_LOG_VERBOSE, &old_output_cfg, NULL);
    }
    if (membuf != NULL) {
        ringbuffer_destroy(&membuf);
    }
    s_log_info("Test result is FAIL");
    return EXIT_FAILURE;
}
