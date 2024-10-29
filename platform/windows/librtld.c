#include "../librtld.h"
#include <core/log.h>
#include <core/util.h>
#include <stdlib.h>
#include <string.h>
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

struct p_sym {
    const char *name;
    void *handle;
};

struct p_lib {
    HMODULE module_handle;
    char *dll_name;
    p_sym_t *syms;
    u32 n_syms;
};

#define DLL_SUFFIX ".dll"

struct p_lib * p_librtld_load(const char *libname, const char **symnames)
{
    struct p_lib *lib = calloc(1, sizeof(struct p_lib));
    s_assert(lib != NULL, "calloc() failed for struct lib");

    lib->dll_name = malloc(strlen(libname) + u_strlen(DLL_SUFFIX) + 1);
    s_assert(lib->dll_name != NULL, "malloc() failed for dll name string");
    strcpy(lib->dll_name, libname);
    strcat(lib->dll_name, DLL_SUFFIX);

    lib->module_handle = LoadLibrary(lib->dll_name);
    if (lib->module_handle == NULL)
         goto_error("Failed to load library \"%s\": %s",
            lib->dll_name, get_last_error_msg());

    lib->n_syms = 0;
    const char **curr_sym_name = symnames;
    while (*(curr_sym_name++))
        lib->n_syms++;

    lib->syms = calloc(lib->n_syms, sizeof(p_sym_t));
    s_assert(lib->syms, "calloc() failed for syms array");

    for (u32 i = 0; i < lib->n_syms; i++) {
        lib->syms[i].name = symnames[i];
        lib->syms[i].handle = GetProcAddress(lib->module_handle, symnames[i]);
        if (lib->syms[i].handle == NULL)
            goto_error("Failed to load symbol \"%s\" from library \"%s\": %s",
                symnames[i], lib->dll_name, get_last_error_msg());
    }
    
    s_log_debug("Successfully loaded %u symbols from '%s'",
        lib->n_syms, lib->dll_name);
    return lib;

err:
    p_librtld_close(&lib);
    return NULL;
}

void p_librtld_close(struct p_lib **lib_p)
{
    if (lib_p == NULL || *lib_p == NULL) return;
    struct p_lib *lib = *lib_p;

    if (lib->syms != NULL) free(lib->syms);
    if (lib->module_handle != NULL) FreeLibrary(lib->module_handle);
    if (lib->dll_name != NULL) free(lib->dll_name);
    u_nzfree(lib_p);
}

void * p_librtld_get_sym_handle(struct p_lib *lib, const char *symname)
{
    u_check_params(lib != NULL && symname != NULL);

    for (u32 i = 0; i < lib->n_syms; i++) {
        if (!strcmp(lib->syms[i].name, symname))
            return lib->syms[i].handle;
    }

    s_log_error("No symbol \"%s\" in library \"%s\"", symname, lib->dll_name);
    return NULL;
}
