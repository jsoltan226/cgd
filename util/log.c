#include "log.h"

#include "int.h"
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

static FILE *out_log_file = NULL, *err_log_file = NULL;
static bool user_fault = NO_USER_FAULT;
static s_log_level current_log_level = LOG_INFO;

void s_log(s_log_level level, const char *module_name, const char *fmt, ...)
{
    if (level < current_log_level)
        return;

    if (err_log_file == NULL) {
        s_set_log_err_filep(stderr);
        s_log_warn("log", "The error log file was unset; setting it to stderr", NULL);
    }
    if (out_log_file == NULL) {
        s_set_log_out_filep(stdout);
        s_log_warn("log", "The out log file was unset; setting it to stdout", NULL);
    }

    FILE *fp = level >= LOG_WARNING ? err_log_file : out_log_file;
    
    if (level == LOG_WARNING) fprintf(fp, "WARNING: ");
    else if (level == LOG_ERROR) fprintf(fp, "ERROR: ");

    fprintf(fp, "[%s] ", module_name);

    va_list vArgs;
    va_start(vArgs, fmt);
    vfprintf(fp, fmt, vArgs);
    va_end(vArgs);
    fprintf(fp, "\n");
}

i32 s_set_log_out_file(const char *file_path)
{
    out_log_file = fopen(file_path, "wb");
    if (out_log_file == NULL) {
        s_log_error("log", "Failed to open out log file '%s': %s", file_path, strerror(errno));
        return 1;
    }

    return 0;
}

i32 s_set_log_out_filep(FILE *fp)
{
    if (fp == NULL) {
        s_log_warn("log", "Not changing out log file to NULL", NULL);
        return 1;
    }
    out_log_file = fp;
    return 0;
}

i32 s_set_log_err_file(const char *file_path)
{
    err_log_file = fopen(file_path, "wb");
    if (err_log_file == NULL) {
        s_log_error("log", "Failed to open error log file '%s': %s", file_path, strerror(errno));
        return 1;
    }

    return 0;
}

i32 s_set_log_err_filep(FILE *fp)
{
    if (fp == NULL) {
        s_log_warn("log", "Not changing error log file to NULL", NULL);
        return 1;
    }
    err_log_file = fp;
    return 0;
}

void s_set_log_level(s_log_level new_log_level)
{
    current_log_level = new_log_level;
}

s_log_level s_get_log_level()
{
    return current_log_level;
}

void s_set_user_fault(bool is_user_fault)
{
    user_fault = is_user_fault;
}

bool s_get_user_fault()
{
    return user_fault;
}

void s_print_usage(bool error, const char *program_name)
{
    fprintf(error ? stderr : stdout, 
            "Usage: %s </path/to/in_image.png> <path/to/out_image.png>\n",
            program_name
           );
}

