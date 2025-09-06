#define S_LOG_LEVEL_LIST_DEF__
#include <core/log.h>
#undef S_LOG_LEVEL_LIST_DEF__
#include <core/int.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#define DEFAULT_LOG_LEVEL S_LOG_INFO

extern int cgd_main(int argc, char **argv);

#ifndef CGD_P_ENTRY_POINT_DEFAULT_LOG_FILEPATH
#define CGD_P_ENTRY_POINT_DEFAULT_LOG_FILEPATH "log.txt"
#endif /* CGD_P_ENTRY_POINT_DEFAULT_LOG_FILEPATH */
#ifndef CGD_P_ENTRY_POINT_DEFAULT_NO_LOG_FILE
#define CGD_P_ENTRY_POINT_DEFAULT_NO_LOG_FILE true
#endif /* CGD_P_ENTRY_POINT_DEFAULT_NO_LOG_FILE */
#ifndef CGD_P_ENTRY_POINT_DEFAULT_LOG_LEVEL
#define CGD_P_ENTRY_POINT_DEFAULT_LOG_LEVEL (S_LOG_INFO)
#endif /* CGD_P_ENTRY_POINT_DEFAULT_LOG_LEVEL */
#ifndef CGD_P_ENTRY_POINT_DEFAULT_LOG_APPEND
#define CGD_P_ENTRY_POINT_DEFAULT_LOG_APPEND true
#endif /* CGD_P_ENTRY_POINT_DEFAULT_LOG_APPEND */
#ifndef CGD_P_ENTRY_POINT_DEFAULT_VERBOSE_LOG_SETUP
#define CGD_P_ENTRY_POINT_DEFAULT_VERBOSE_LOG_SETUP true
#endif /* CGD_P_ENTRY_POINT_DEFAULT_VERBOSE_LOG_SETUP */

static i32 setup_log(FILE **o_out_log_fp, FILE **o_err_log_fp,
    const char *filepath, enum s_log_level log_level, bool append);
static void cleanup_log(FILE **out_log_fp_p, FILE **err_log_fp_p);

static bool get_env_bool(const char *envvar, bool default_val);
static const char *get_env_str(const char *envvar, const char *default_val);
static enum s_log_level get_env_log_level(enum s_log_level default_val);

int main(int argc, char **argv)
{
    FILE *out_log_fp = NULL, *err_log_fp = NULL;

    const char *log_filepath = get_env_str("CGD_LOG_FILE",
        CGD_P_ENTRY_POINT_DEFAULT_LOG_FILEPATH);

    if (get_env_bool("CGD_NO_LOG_FILE", CGD_P_ENTRY_POINT_DEFAULT_NO_LOG_FILE))
        log_filepath = NULL;

    enum s_log_level log_level =
        get_env_log_level(CGD_P_ENTRY_POINT_DEFAULT_LOG_LEVEL);

    bool log_append = get_env_bool("CGD_LOG_APPEND",
        CGD_P_ENTRY_POINT_DEFAULT_LOG_APPEND);

    bool verbose_log_setup = get_env_bool("CGD_VERBOSE_LOG_SETUP",
        CGD_P_ENTRY_POINT_DEFAULT_VERBOSE_LOG_SETUP);

    if (setup_log(&out_log_fp, &err_log_fp,
            log_filepath, log_level, log_append))
    {
        fprintf(stderr, "Failed to set up logging. Stop.\n");
        return EXIT_FAILURE;
    }

    if (verbose_log_setup) {
        const char *old_line = NULL;
        s_configure_log_line(S_LOG_VERBOSE, "%s", &old_line);
        s_log(S_LOG_VERBOSE, "", "\nLog setup OK. Calling main...\n");
        s_configure_log_line(S_LOG_VERBOSE, old_line, NULL);
    }

    int ret = cgd_main(argc, argv);

    cleanup_log(&out_log_fp, &err_log_fp);

    return ret;
}

static i32 setup_log(FILE **o_out_log_fp, FILE **o_err_log_fp,
    const char *filepath, enum s_log_level log_level, bool append)
{
    s_configure_log_level(log_level);

    *o_out_log_fp = *o_err_log_fp = NULL;
    FILE *out_log_fp = stdout, *err_log_fp = stderr;

    if (filepath != NULL) {
        FILE *custom_fp = fopen(filepath, append ? "ab" : "wb");
        if (custom_fp == NULL) {
            fprintf(stderr, "Failed to open log file \"%s\": %s. "
                "Falling back to stdout/stderr.\n",
                filepath, strerror(errno));

        } else {
            out_log_fp = err_log_fp = custom_fp;
        }
    }

    /* Set the strip esc sequences flags if we're not writing to a terminal */
    struct s_log_output_cfg output_cfg = {
        .type = S_LOG_OUTPUT_FILE,
        .out.file = out_log_fp,
        .flags = S_LOG_CONFIG_FLAG_COPY |
            (out_log_fp != stdout ? S_LOG_CONFIG_FLAG_STRIP_ESC_SEQUENCES : 0),
    };
    i32 ret = s_configure_log_outputs(S_LOG_STDOUT_MASKS, &output_cfg);
    if (ret != 0) {
        fprintf(stderr, "Failed to configure %d stdout log output(s)\n", ret);
        return 1;
    }

    output_cfg.out.file = err_log_fp;
    if (err_log_fp != stderr)
        output_cfg.flags |= S_LOG_CONFIG_FLAG_STRIP_ESC_SEQUENCES;
    else
        output_cfg.flags &= ~S_LOG_CONFIG_FLAG_STRIP_ESC_SEQUENCES;

    ret = s_configure_log_outputs(S_LOG_STDERR_MASKS, &output_cfg);
    if (ret != 0) {
        fprintf(stderr, "Failed to configure %d stderr log output(s)\n", ret);
        return 1;
    }

    *o_out_log_fp = out_log_fp;
    *o_err_log_fp = err_log_fp;

    return 0;
}

static void cleanup_log(FILE **out_log_fp_p, FILE **err_log_fp_p)
{
    s_log_cleanup_all();

    if (*out_log_fp_p != NULL && *out_log_fp_p != stdout) {
        if (fclose(*out_log_fp_p)) {
            fprintf(stderr, "Failed to close the out log file: %s\n",
                strerror(errno));
        }
    }
    if (*err_log_fp_p != NULL && *err_log_fp_p != stderr &&
        *err_log_fp_p != *out_log_fp_p)
    {
        if (fclose(*err_log_fp_p)) {
            fprintf(stderr, "Failed to close the error log file: %s\n",
                strerror(errno));
        }
    }
    *out_log_fp_p = *err_log_fp_p = NULL;
}

static bool get_env_bool(const char *envvar, bool default_val)
{
    const char *val = getenv(envvar);
    if (val)
        return (!strcasecmp(val, "true") || !strcmp(val, "1"));
    else
        return default_val;
}

static const char *get_env_str(const char *envvar, const char *default_val)
{
    const char *val = getenv(envvar);
    if (val)
        return val;
    else
        return default_val;
}

static enum s_log_level get_env_log_level(enum s_log_level default_val)
{
#define X_(name) #name,
    static const char *const log_level_strings[S_LOG_N_LEVELS_] = {
        S_LOG_LEVEL_LIST
    };
#undef X_
    enum s_log_level ret = default_val;
    const char *log_level_env = getenv("CGD_LOG_LEVEL");
    if (log_level_env != NULL) {
        for (u32 i = 0; i < S_LOG_N_LEVELS_; i++) {
            if (!strcmp(log_level_strings[i], log_level_env)) {
                ret = i;
                return ret;
            }
        }

        if (!strcmp("S_LOG_DISABLED", log_level_env)) {
            ret = S_LOG_DISABLED;
            return ret;
        }
    }

    return default_val;
}
