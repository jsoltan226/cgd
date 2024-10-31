#include "../librtld.h"
#include <core/log.h>
#include <core/util.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <string.h>

struct p_sym {
    const char *name;
    void *handle;
};

struct p_lib {
    char *so_name;
    void *handle;
    p_sym_t *syms;
    u32 n_syms;
};

#define MODULE_NAME "librtld"

#define SO_EXT ".so"

struct p_lib * p_librtld_load(const char *libname, const char **symnames)
{
    u_check_params(libname != NULL && symnames != NULL);

    struct p_lib *lib = calloc(1, sizeof(struct p_lib));
    s_assert(lib != NULL, "calloc() failed for struct lib");

    lib->so_name = malloc(strlen(libname) + u_strlen(SO_EXT) + 1);
    s_assert(lib != NULL, "malloc() failed for so_name string");
    strcpy(lib->so_name, libname);
    strcat(lib->so_name, SO_EXT);

    lib->handle = dlopen(lib->so_name, RTLD_LAZY);
    if (lib->handle == NULL)
        goto_error("Failed to dlopen() lib '%s': %s", lib->so_name, dlerror());

    char **curr_symname = (char **)symnames;
    while (*(curr_symname++) != NULL)
        lib->n_syms++;
    
    lib->syms = calloc(lib->n_syms, sizeof(p_sym_t));
    s_assert(lib->syms != NULL, "calloc() failed for lib syms");
    
    for (u32 i = 0; i < lib->n_syms; i++) {
        lib->syms[i].handle = dlsym(lib->handle, symnames[i]);
        if (lib->syms[i].handle == NULL)
            goto_error("Failed to look up symbol \"%s\" in lib \"%s\": %s",
                symnames[i], lib->so_name, dlerror());

        lib->syms[i].name = symnames[i];
    }

    s_log_debug("Successfully loaded %u symbols from \"%s\"",
        lib->n_syms, lib->so_name);

    return lib;

err:
    p_librtld_close(&lib);
    return NULL;
}

void p_librtld_close(struct p_lib **lib_p)
{
    if (lib_p == NULL || *lib_p == NULL) return;
    struct p_lib *lib = *lib_p;

    if (lib->handle != NULL) {
        dlclose(lib->handle);
        lib->handle = NULL;
    }
    if (lib->so_name != NULL) u_nfree(&lib->so_name);
    if (lib->syms != NULL) u_nfree(&lib->syms);

    u_nzfree(lib_p);
}

void * p_librtld_get_sym_handle(struct p_lib *lib, const char *symname)
{
    u_check_params(lib != NULL && symname != NULL);
    for (u32 i = 0; i < lib->n_syms; i++) {
        if (!strcmp(lib->syms[i].name, symname))
            return lib->syms[i].handle;
    }
    s_log_error("%s(): Could not find symbol \"%s\" in lib \"%s\"",
        __func__, symname, lib->so_name);
    return NULL;
}
