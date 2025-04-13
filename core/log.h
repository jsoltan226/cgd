#ifndef S_LOG_H_
#define S_LOG_H_
#include "static-tests.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdnoreturn.h>
#include "int.h"

#define S_LOG_LEVEL_LIST    \
    X_(S_LOG_TRACE)         \
    X_(S_LOG_DEBUG)         \
    X_(S_LOG_VERBOSE)       \
    X_(S_LOG_INFO)          \
    X_(S_LOG_WARNING)       \
    X_(S_LOG_ERROR)         \
    X_(S_LOG_FATAL_ERROR)   \

#define X_(name) name,
enum s_log_level {
    S_LOG_DISABLED = -1,
    S_LOG_LEVEL_LIST
    S_LOG_N_LEVELS_
};
#undef X_

#define X_(name) name##_MASK = 1 << name,
enum s_log_level_mask {
    S_LOG_LEVEL_LIST
};
#undef X_
static_assert(sizeof(enum s_log_level_mask) <= sizeof(u32),
    "The size of the log level mask enum must be within the size of a u32.");

#define S_LOG_STDOUT_MASKS (S_LOG_TRACE_MASK | S_LOG_DEBUG_MASK \
        | S_LOG_VERBOSE_MASK | S_LOG_INFO_MASK)
#define S_LOG_STDERR_MASKS (S_LOG_WARNING_MASK | S_LOG_ERROR_MASK \
        | S_LOG_FATAL_ERROR_MASK)
#define S_LOG_ALL_MASKS (S_LOG_STDOUT_MASKS | S_LOG_STDERR_MASKS)

#ifndef S_LOG_LEVEL_LIST_DEF__
#undef S_LOG_LEVEL_LIST
#endif /* S_LOG_LEVEL_LIST_DEF__ */

void s_log(enum s_log_level level, const char *module_name,
    const char *fmt, ...);

#ifndef CGD_BUILDTYPE_RELEASE
#define s_log_trace(...) s_log(S_LOG_TRACE, MODULE_NAME, __VA_ARGS__)
#define s_log_debug(...) s_log(S_LOG_DEBUG, MODULE_NAME, __VA_ARGS__)
#else
#define s_log_trace(...)
#define s_log_debug(...)
#endif /* NDEBUG */

#define s_log_verbose(...) s_log(S_LOG_VERBOSE, MODULE_NAME, __VA_ARGS__)

#define s_log_info(...) s_log(S_LOG_INFO, MODULE_NAME, __VA_ARGS__)

#define s_log_warn(...) s_log(S_LOG_WARNING, MODULE_NAME, __VA_ARGS__)

#define s_log_error(...) s_log(S_LOG_ERROR, MODULE_NAME, __VA_ARGS__)

noreturn void s_abort(const char *module_name, const char *function_name,
    const char *error_msg_fmt, ...);
#define s_log_fatal(...) s_abort(MODULE_NAME, __func__, __VA_ARGS__)

#define s_assert(expr, /* msg on fail */...) if (!(expr)) {                 \
    s_log_error("Assertion failed: '%s'", #expr);                           \
    s_log_fatal(__VA_ARGS__);                                               \
}
#define s_assert_and_eval(expr, /* msg on fail */...) ((expr) ? 1 : do {    \
    s_log_error("Assertion failed: '%s'", #expr);                           \
    s_log_fatal(__VA_ARGS__);                                               \
} while (0))

void s_configure_log_level(enum s_log_level new_log_level);
enum s_log_level s_get_log_level(void);

struct s_log_output_cfg {
    enum s_log_output_type {
        S_LOG_OUTPUT_FILE,
        S_LOG_OUTPUT_FILEPATH,
        S_LOG_OUTPUT_MEMORYBUF,
        S_LOG_OUTPUT_NONE,
    } type;
    union {
        FILE *file;
        const char *filepath;

        struct s_log_output_memorybuf {
#define S_LOG_DEFAULT_MEMBUF_SIZE 4096
#define S_LOG_MINIMAL_MEMBUF_SIZE 16
            char *buf;
            u64 buf_size;
        } membuf;
    } out;

    bool flag_append;
    bool flag_copy;
};
i32 s_configure_log_output(enum s_log_level level,
    const struct s_log_output_cfg *in_new_cfg,
    struct s_log_output_cfg *out_old_cfg);

i32 s_configure_log_outputs(u32 level_mask, const struct s_log_output_cfg *cfg);

#define S_LOG_PREFIX_SIZE 32
void s_configure_log_prefix(enum s_log_level level,
    char in_new_prefix[S_LOG_PREFIX_SIZE], /* Must not be local-scope */
    char out_old_prefix[S_LOG_PREFIX_SIZE],
    bool strip_esc_sequences);

void s_log_cleanup_all(void);

#endif /* S_LOG_H_ */
