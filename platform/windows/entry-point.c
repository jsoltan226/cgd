#define P_INTERNAL_GUARD__
#include "global.h"
#undef P_INTERNAL_GUARD__
#define P_INTERNAL_GUARD__
#include "error.h"
#undef P_INTERNAL_GUARD__
#define S_LOG_LEVEL_LIST_DEF__
#include <core/log.h>
#undef S_LOG_LEVEL_LIST_DEF__
#include <core/int.h>
#include <core/util.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif /* WIN32_LEAN_AND_MEAN */
#include <windows.h>
#include <winnls.h>
#include <shellapi.h>
#include <minwindef.h>
#include <stringapiset.h>

extern int cgd_main(int argc, char **argv);

#undef goto_error
#define goto_error(...) do {        \
    fprintf(stderr, __VA_ARGS__);   \
    goto err;                       \
} while (0)

#ifndef CGD_P_ENTRY_POINT_DEFAULT_LOG_FILEPATH
#define CGD_P_ENTRY_POINT_DEFAULT_LOG_FILEPATH "log.txt"
#endif /* CGD_P_ENTRY_POINT_DEFAULT_LOG_FILEPATH */
#ifndef CGD_P_ENTRY_POINT_DEFAULT_NO_LOG_FILE
#define CGD_P_ENTRY_POINT_DEFAULT_NO_LOG_FILE false
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

static i32 prepare_cmdline(i32 *o_argc, char ***o_argv, LPWSTR lpCmdLine);
static i32 setup_log(FILE **o_log_fp, const char *filepath,
    enum s_log_level log_level, bool append);

static bool get_env_bool(const char *envvar, bool default_val);
static const char *get_env_str(const char *envvar, const char *default_val);
static enum s_log_level get_env_log_level(enum s_log_level default_val);

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    LPWSTR lpCmdLine, int nShowCmd)
{
    (void) hPrevInstance;

    i32 argc = 0;
    char **argv = NULL;
    FILE *log_fp = NULL;

    /** INITIALIZE GLOBAL VARIABLES **/
    g_instance_handle = hInstance;
    g_n_cmd_show = nShowCmd;

    /** READ THE ENVIRONMENT **/
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

    /** PREPARE THE COMMAND LINE **/
    if (prepare_cmdline(&argc, &argv, lpCmdLine))
        goto_error("Failed to prepare the command line. Stop.\n");

    /* Set up logging */
    if (setup_log(&log_fp, log_filepath, log_level, log_append))
        goto_error("Failed to set up logging. Stop.\n");

    if (verbose_log_setup) {
        const char *old_line = NULL;
        s_configure_log_line(S_LOG_VERBOSE, "%s", &old_line);
        s_log(S_LOG_VERBOSE, "", "\nLog setup OK. Calling main...\n");
        s_configure_log_line(S_LOG_VERBOSE, old_line, NULL);
    }

    /** CALL MAIN **/
    int ret = cgd_main(argc, argv);

    /** CLEANUP **/
    s_log_cleanup_all();
    if (log_fp != NULL) {
        if (fclose(log_fp))
            fprintf(stderr, "Failed to close the log file: %s\n", strerror(errno));
    }

    for (i32 i = 0; i < argc; i++)
        u_nfree(&argv[i]);
    u_nfree(&argv);

    return ret;

err:
    if (log_fp != NULL) {
        if (fclose(log_fp)) {
            fprintf(stderr, "Couldn't close the log file: %s\n",
                strerror(errno));
        }
        log_fp = NULL;
    }
    if (argv != NULL) {
        for (i32 i = 0; i < argc; i++) {
            if (argv[i] != NULL)
                u_nfree(&argv[i]);
        }
        u_nfree(&argv);
    }
    return EXIT_FAILURE;
}

static i32 prepare_cmdline(i32 *o_argc, char ***o_argv, LPWSTR lpCmdLine)
{
    wchar_t **argv_w = NULL;
    i32 argc = 0;
    char **argv = NULL;

    /* Convert to argv-style (wide char) command line */
    argv_w = CommandLineToArgvW(lpCmdLine, &argc);
    if (argv_w == NULL)
        goto_error("Failed to convert the command line to argv: %s\n",
            get_last_error_msg());

    /* Allocate the argv array */
    argv = calloc(argc + 1, sizeof(char *));
    if (argv == NULL)
        goto_error("Failed to allocate the argv array\n");

    /* Allocate, copy and convert each argv string */
    for (i32 i = 0; i < argc; i++) {
        /* First just get the size */
        i32 str_size = WideCharToMultiByte(CP_UTF8, 0, argv_w[i],
            -1, NULL, 0, NULL, NULL);
        if (str_size == 0)
            goto_error("Failed to convert string from wchar to UTF-8\n");

        /* Now, allocate the string */
        argv[i] = malloc(str_size);
        if (WideCharToMultiByte(CP_UTF8, 0, argv_w[i], -1,
                argv[i], str_size, NULL, NULL) == 0)
        {
            goto_error("Failed to convert string from wchar to UTF-8\n");
        }
    }
    argv[argc] = NULL;

    (void) LocalFree(argv_w);

    *o_argc = argc;
    *o_argv = argv;
    return 0;

err:
    if (argv != NULL) {
        for (i32 i = 0; i < argc; i++)
            if (argv[i] != NULL) u_nfree(&argv[i]);

        u_nfree(&argv);
    }
    if (argv_w != NULL) {
        (void) LocalFree(argv_w);
        argv_w = NULL;
    }

    *o_argc = 0;
    *o_argv = NULL;
    return 1;
}

static i32 setup_log(FILE **o_log_fp, const char *filepath,
    enum s_log_level log_level, bool append)
{
    s_configure_log_level(log_level);

    if (filepath == NULL) /* No log file */
        return 0;

    FILE *fp = fopen(filepath, append ? "ab" : "wb");
    if (fp == NULL) {
        fprintf(stderr, "Failed to open log file \"%s\": %s\n",
            filepath, strerror(errno));
        *o_log_fp = NULL;
        return 1;
    }

    const struct s_log_output_cfg output_cfg = {
        .type = S_LOG_OUTPUT_FILE,
        .out.file = fp,
        .flags = S_LOG_CONFIG_FLAG_COPY | S_LOG_CONFIG_FLAG_STRIP_ESC_SEQUENCES
    };
    i32 ret = s_configure_log_outputs(S_LOG_ALL_MASKS, &output_cfg);
    if (ret != 0) {
        fprintf(stderr, "Failed to configure %d log output(s)\n", ret);
        if (fp != NULL) {
            if (fclose(fp)) {
                fprintf(stderr, "Failed to close the log file: %s",
                    strerror(errno));
            }
        }
        *o_log_fp = NULL;
        return 1;
    }

    *o_log_fp = fp;

    return 0;
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
