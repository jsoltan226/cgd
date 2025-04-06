#ifndef LOG_H_
#define LOG_H_
#include "static-tests.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdnoreturn.h>
#include "int.h"

typedef enum {
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR,
    LOG_FATAL_ERROR,
} s_log_level;

void s_log(s_log_level level, const char *module_name, const char *fmt, ...);

#ifndef NDEBUG
#define s_log_debug(...) s_log(LOG_DEBUG, MODULE_NAME, __VA_ARGS__)
#else
#define s_log_debug(...)
#endif /* NDEBUG */

#define s_log_info(...) s_log(LOG_INFO, MODULE_NAME, __VA_ARGS__)

#define s_log_warn(...) s_log(LOG_WARNING, MODULE_NAME, __VA_ARGS__)

#define s_log_error(...) s_log(LOG_ERROR, MODULE_NAME, __VA_ARGS__)

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

void s_set_log_level(s_log_level new_log_level);
s_log_level s_get_log_level(void);

i32 s_set_log_out_file(const char *file_path);
i32 s_set_log_out_filep(FILE **fpp);
#define s_set_log_file s_set_log_out_file
#define s_set_log_filep s_set_log_out_filep

i32 s_set_log_err_file(const char *file_path);
i32 s_set_log_err_filep(FILE **fpp);

void s_close_out_log_fp(void);
void s_close_err_log_fp(void);

/* This is used in the error handling system to determine
 * whether the error was the user's fault
 * (e.g a non-existent file was given as input)
 * to decide if usage should be printed after the error message
 */
#define YES_USER_FAULT  true
#define NO_USER_FAULT   false
void s_set_user_fault(bool is_user_fault);
bool s_get_user_fault(void);

/* A shortcut for configuring logging, especially in tests */
#define s_configure_log(level, outfilep, errfilep) do { \
    s_set_log_level(level);                             \
    s_set_log_out_filep(outfilep);                      \
    s_set_log_err_filep(errfilep);                      \
} while (0);

#endif /* LOG_H_ */
