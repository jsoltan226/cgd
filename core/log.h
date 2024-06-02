#ifndef LOG_H_
#define LOG_H_

#include <stdbool.h>
#include <stdio.h>
#include "int.h"

typedef enum {
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR
} s_log_level;

void s_log(s_log_level level, const char *module_name, const char *fmt, ...);

#ifndef NDEBUG
#define s_log_debug(module_name, fmt...) \
    s_log(LOG_DEBUG, module_name, fmt)
#else
#define s_log_debug(module_name, fmt...)
#endif /* NDEBUG */

#define s_log_info(module_name, fmt...) \
    s_log(LOG_INFO, module_name, fmt)

#define s_log_warn(module_name, fmt...) \
    s_log(LOG_WARNING, module_name, fmt)

#define s_log_error(module_name, fmt...) \
    s_log(LOG_ERROR, module_name, fmt)

void s_set_log_level(s_log_level new_log_level);
s_log_level s_get_log_level();

i32 s_set_log_out_file(const char *file_path);
i32 s_set_log_out_filep(FILE *fp);
#define s_set_log_file s_set_log_out_file
#define s_set_log_filep s_set_log_out_filep

i32 s_set_log_err_file(const char *file_path);
i32 s_set_log_err_filep(FILE *fp);

/* This is used in the error handling system to determine
 * whether the error was the user's fault (e.g a non-existent file was given as input)
 * to decide if usage should be printed after the error message
 */
#define YES_USER_FAULT  true
#define NO_USER_FAULT   false
void s_set_user_fault(bool is_user_fault);
bool s_get_user_fault();

#endif /* LOG_H_ */
