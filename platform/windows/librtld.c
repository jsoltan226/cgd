#include "../librtld.h"
#include <core/log.h>
#include <core/util.h>
#include <core/vector.h>
#include <errhandlingapi.h>
#include <stdlib.h>
#include <string.h>
#include <winerror.h>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif /* WIN32_LEAN_AND_MEAN */
#include <windows.h>
#include <winnt.h>
#include <minwindef.h>
#include <libloaderapi.h>
#define P_INTERNAL_GUARD__
#include "error.h"
#undef P_INTERNAL_GUARD__

#define MODULE_NAME "librtld"

#define DEFAULT_SUFFIX ".dll"
#define DEFAULT_PREFIX "lib"

struct sym {
    char *name;
    void *handle;
};

struct p_lib {
    HMODULE module_handle;
    char *dll_name;
    VECTOR(struct sym) syms;
};

struct p_lib * p_librtld_load(const char *libname, const char *const *symnames)
{
    u_check_params(libname != NULL && symnames != NULL);

    struct p_lib *lib = p_librtld_load_lib_explicit(libname, NULL, NULL, NULL);
    if (lib == NULL)
        goto err;

    u32 i = 0;
    while (symnames[i] != NULL) {
        if (p_librtld_load_sym(lib, symnames[i]) == NULL)
            goto err;

        i++;
    }

    s_log_verbose("Successfully loaded %u symbols from \"%s\"",
        vector_size(lib->syms), lib->dll_name);

    return lib;

err:
    p_librtld_close(&lib);
    return NULL;
}

struct p_lib * p_librtld_load_lib_explicit(const char *libname,
    const char *prefix, const char *suffix, const char *version_string)
{
    u_check_params(libname != NULL);

    struct p_lib *lib = calloc(1, sizeof(struct p_lib));
    s_assert(lib != NULL, "calloc() failed for struct lib");

    u32 total_module_name_size = strlen(libname);

    if (prefix == NULL) prefix = DEFAULT_PREFIX;
    total_module_name_size += strlen(prefix);

    if (suffix == NULL) suffix = DEFAULT_SUFFIX;
    total_module_name_size += strlen(suffix);

    if (version_string != NULL)
        total_module_name_size += strlen(version_string) + u_strlen("-");

    total_module_name_size++; /* The NULL terminator */

    lib->dll_name = malloc(total_module_name_size);
    s_assert(lib->dll_name != NULL, "malloc() failed for dll_name string");
    lib->dll_name[0] = '\0'; /* For `strcat` to not trip over */

    /* Check if the provided library name already starts with the prefix */
    bool prefix_already_provided_in_libname = true;
    if (strncmp(libname, prefix, strlen(prefix))) {
        strcat(lib->dll_name, prefix);
        prefix_already_provided_in_libname = false;
    }

    /* Append the library name and extension */
    strcat(lib->dll_name, libname);

    /* If there is a version string, append it prefixed with a '-'
     * (e.g. NULL, "vulkan", ".dll", "1" -> "vulkan-1.dll") */
    if (version_string != NULL) {
        strcat(lib->dll_name, "-");
        strcat(lib->dll_name, version_string);
    }

    /* Finally, append the suffix */
    strcat(lib->dll_name, suffix);

    /* Open the library */
    lib->module_handle = LoadLibraryA(lib->dll_name);
    if (lib->module_handle == NULL) {
        /* If the provided libname didn't include the prefix
         * (e.g. "test" instead of "libtest"), we should now try to load
         * the version WITHOUT the prefix (the one that was given in the first
         * place lol */
        if (!prefix_already_provided_in_libname) {
            /* Cut off the existing prefix */
            char *new_dll_name = malloc(total_module_name_size - strlen(prefix));
            s_assert(new_dll_name != NULL, "malloc failed for new dll_name");
            strcpy(new_dll_name, lib->dll_name + strlen(prefix));
            free(lib->dll_name);
            lib->dll_name = new_dll_name;

            lib->module_handle = LoadLibraryA(lib->dll_name);
            if (lib->module_handle == NULL)
                 goto_error("Failed to load library \"%s\": %s",
                    lib->dll_name, get_last_error_msg());
        } else {
             goto_error("Failed to load library \"%s\": %s",
                lib->dll_name, get_last_error_msg());
        }
    }

    /* Log the actual path to the library */
    char buf[PATH_MAX] = { 0 };
    if (GetModuleFileNameA(lib->module_handle, buf, sizeof(buf)) == 0)
        goto_error("Failed to get the library's path: %s",
            get_last_error_msg());
    if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
        s_log_warn("The dll's path string was truncated "
            "due to insufficient buffer size");

    s_log_verbose("%s -> %s", lib->dll_name, buf);

    lib->syms = vector_new(struct sym);

    return lib;
err:
    p_librtld_close(&lib);
    return NULL;
}

void * p_librtld_load_sym(struct p_lib *lib, const char *symname)
{
    u_check_params(lib != NULL && symname != NULL);

    /* Search the lib's sym list first */
    for (u32 i = 0; i < vector_size(lib->syms); i++) {
        if (!strcmp(lib->syms[i].name, symname))
            return lib->syms[i].handle;
    }

    /* If nothing's found in the symlist, try to load it from the lib itself */
    const union {
        FARPROC win32;
        void *p_librtld;
    } bridge = {
        .win32 = GetProcAddress(lib->module_handle, symname),
    };
    if (bridge.win32 == NULL) {
        s_log_error("Failed to load sym \"%s\" from lib \"%s\": %s",
            symname, lib->dll_name, get_last_error_msg());
        return NULL;
    }
    void *const sym_handle = bridge.p_librtld;

    /* Add the new symbol to the sym list */
    struct sym new_sym = {
        .name = _strdup(symname),
        .handle = sym_handle
    };
    s_assert(new_sym.name != NULL, "strdup(symname) returned NULL");
    if (lib->syms == NULL)
        lib->syms = vector_new(struct sym);
    vector_push_back(&lib->syms, new_sym);

    return sym_handle;
}

void p_librtld_close(struct p_lib **lib_p)
{
    if (lib_p == NULL || *lib_p == NULL) return;
    struct p_lib *lib = *lib_p;

    if (lib->module_handle != NULL) {
        if (FreeLibrary(lib->module_handle) == 0)
            s_log_error("Failed to free the library \"%s\": %s",
                lib->dll_name ? lib->dll_name : "<N/A>", get_last_error_msg());
        lib->module_handle = NULL;
    }
    if (lib->dll_name != NULL) {
        u_nfree(&lib->dll_name);
    }
    if (lib->syms != NULL) {
        for (u32 i = 0; i < vector_size(lib->syms); i++) {
            if (lib->syms[i].name != NULL)
                free(lib->syms[i].name);
        }
        vector_destroy(&lib->syms);
    }

    u_nzfree(lib_p);
}
