#include "librtld.h"
#include "log.h"
#include "util.h"
#include <stdlib.h>
#include <dlfcn.h>
#include <string.h>

#define MODULE_NAME "librtld"

struct lib * librtld_load(const char *libname, const char **symnames)
{
    u_check_params(libname != NULL && symnames != NULL);

    struct lib *lib = calloc(1, sizeof(struct lib));
    s_assert(lib != NULL, "calloc() failed for struct lib");

    lib->handle = dlopen(libname, RTLD_LAZY);
    if (lib->handle == NULL)
        goto_error("Failed to dlopen() lib '%s': %s", libname, dlerror());

    char **curr_symname = (char **)symnames;
    while (*curr_symname != NULL) {
        lib->n_syms++;
        curr_symname += 1;
    }
    
    lib->syms = calloc(lib->n_syms, sizeof(sym_t));
    s_assert(lib->syms != NULL, "calloc() failed for lib syms");
    
    for (u32 i = 0; i < lib->n_syms; i++) {
        lib->syms[i].handle = dlsym(lib->handle, symnames[i]);
        if (lib->syms[i].handle == NULL)
            goto_error("Failed to dlsym() symbol '%s' in lib '%s': %s",
                symnames[i], libname, dlerror());

        lib->syms[i].name = symnames[i];
    }

    s_log_debug("Successfully loaded %u symbols from '%s'", lib->n_syms, libname);

    return lib;

err:
    if (lib->handle != NULL) dlclose(lib->handle);
    if (lib->syms != NULL) u_nzfree(lib->syms);
    u_nzfree(lib);
    return NULL;
}

void librtld_close(struct lib *lib)
{
    if (lib == NULL) return;
    dlclose(lib->handle);
    free(lib->syms);
    free(lib);
}

void * librtld_get_sym_handle(struct lib *lib, const char *symname)
{
    u_check_params(lib != NULL && symname != NULL);
    for (u32 i = 0; i < lib->n_syms; i++) {
        if (!strcmp(lib->syms[i].name, symname))
            return lib->syms[i].handle;
    }
    return NULL;
}
