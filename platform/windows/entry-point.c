#include "core/util.h"
#include <core/int.h>
#include <core/util.h>
#include <stdio.h>
#include <wchar.h>
#include <stdlib.h>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif /* WIN32_LEAN_AND_MEAN */
#include <windows.h>
#include <winnls.h>
#include <shellapi.h>
#include <minwindef.h>
#include <stringapiset.h>
#define P_INTERNAL_GUARD__
#include "global.h"
#undef P_INTERNAL_GUARD__
#define P_INTERNAL_GUARD__
#include "error.h"
#undef P_INTERNAL_GUARD__

extern int cgd_main(int argc, char **argv);

#undef goto_error
#define goto_error(...) do {        \
    fprintf(stderr, __VA_ARGS__);   \
    goto err;                       \
} while (0)

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    LPWSTR lpCmdLine, int nShowCmd)
{
    /** INITIALIZE GLOBAL VARIABLES **/
    g_instance_handle = hInstance;
    g_n_cmd_show = nShowCmd;

    /** PREPARE THE COMMAND LINE **/
    int argc = 0;
    wchar_t **argv_w = NULL;
    char **argv = NULL;

    /* Convert to argv-style (wide char) command line */
    argv_w = CommandLineToArgvW(lpCmdLine, &argc);
    if (argv_w == NULL)
        goto_error("Failed to convert the command line to argv: %s. Stop.\n",
            get_last_error_msg());

    /* Allocate the argv array */
    argv = calloc(argc, sizeof(char *));
    if (argv == NULL)
        goto_error("Failed to allocate the argv array. Stop.\n");

    /* Allocate, copy and convert each argv string */
    for (u32 i = 0; i < argc; i++) {
        /* First just get the size */
        i32 str_size = WideCharToMultiByte(CP_UTF8, 0, argv_w[i],
            -1, NULL, 0, NULL, NULL);
        if (str_size == 0)
            goto_error("Failed to convert string from wchar to char. Stop.\n");

        /* Now, allocate the string */
        argv[i] = malloc(str_size);
        if (WideCharToMultiByte(CP_UTF8, 0, argv_w[i], -1,
                argv[i], str_size, NULL, NULL) == 0)
        {
            goto_error("Failed to convert string from wchar to char. Stop.\n");
        }
    }

    /** CALL MAIN **/
    int ret = cgd_main(argc, argv);

    /** CLEANUP **/
    for (u32 i = 0; i < argc; i++)
        free(argv[i]);
    u_nzfree(argv);

    LocalFree(argv_w);
    argv_w = NULL;

    return ret;

err:
    if (argv != NULL) {
        for (u32 i = 0; i < argc; i++) {
            if (argv[i] != NULL) free(argv[i]);
        }
        u_nzfree(argv);
    }
    if (argv_w != NULL) {
        LocalFree(argv_w);
        argv_w = NULL;
    }
    return EXIT_FAILURE;
}
