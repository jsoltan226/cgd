#define _GNU_SOURCE
#include "../librtld.h"
#include <core/log.h>
#include <core/util.h>
#include <core/vector.h>
#include <stdlib.h>
#include <string.h>
#include <link.h>
#include <dlfcn.h>

#define MODULE_NAME "librtld"

#define DEFAULT_SUFFIX ".so"
#define DEFAULT_PREFIX "lib"

struct sym {
    char *name;
    void *handle;
};

struct p_lib {
    char *so_name;
    void *handle;
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
        vector_size(lib->syms), lib->so_name);

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

    u32 total_so_name_size = strlen(libname);

    if (prefix == NULL) prefix = DEFAULT_PREFIX;
    total_so_name_size += strlen(prefix);

    if (suffix == NULL) suffix = DEFAULT_SUFFIX;
    total_so_name_size += strlen(suffix);

    if (version_string != NULL)
        total_so_name_size += strlen(version_string) + u_strlen(".");

    total_so_name_size++; /* The NULL terminator */

    lib->so_name = malloc(total_so_name_size);
    s_assert(lib->so_name != NULL, "malloc() failed for so_name string");
    lib->so_name[0] = '\0'; /* For `strcat` to not trip over */

    /* Check if the provided library name already starts with the prefix */
    bool prefix_already_provided_in_libname = true;
    if (strncmp(libname, prefix, strlen(prefix))) {
        strcat(lib->so_name, prefix);
        prefix_already_provided_in_libname = false;
    }

    /* Append the library name and extension */
    strcat(lib->so_name, libname);
    strcat(lib->so_name, suffix);

    /* If there is a version string, append it prefixed with a '.' */
    if (version_string != NULL) {
        strcat(lib->so_name, ".");
        strcat(lib->so_name, version_string);
    }

    /* Open the library */
    lib->handle = dlopen(lib->so_name, RTLD_LAZY);
    if (lib->handle == NULL) {
        /* If the provided libname didn't include the prefix
         * (e.g. "test" instead of "libtest"), we should now try to load
         * the version WITHOUT the prefix (the one that was given in the first
         * place lol */
        if (!prefix_already_provided_in_libname) {
            /* Cut off the existing prefix */
            char *new_so_name = malloc(total_so_name_size - strlen(prefix));
            s_assert(new_so_name != NULL, "malloc failed for new so_name");
            strcpy(new_so_name, lib->so_name + strlen(prefix));
            free(lib->so_name);
            lib->so_name = new_so_name;

            lib->handle = dlopen(lib->so_name, RTLD_LAZY);
            if (lib->handle == NULL)
                goto_error("Failed to dlopen() lib '%s': %s",
                    lib->so_name, dlerror());
        } else {
            goto_error("Failed to dlopen() lib '%s': %s",
                lib->so_name, dlerror());
        }
    }

    lib->syms = vector_new(struct sym);

    /* Log the full path to the shared object */
    struct link_map *lm_p;
    if (dlinfo(lib->handle, RTLD_DI_LINKMAP, &lm_p))
        goto_error("dlinfo failed on \"%s\": %s", lib->so_name, dlerror());
    s_log_verbose("%s -> %s", lib->so_name, lm_p->l_name);

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
    void *sym_handle = dlsym(lib->handle, symname);
    if (sym_handle == NULL) {
        s_log_error("Failed to load sym \"%s\" from lib \"%s\": %s",
            symname, lib->so_name, dlerror());
        return NULL;
    }

    /* Add the new symbol to the sym list */
    struct sym new_sym = {
        .name = strdup(symname),
        .handle = sym_handle
    };
    s_assert(new_sym.name != NULL, "strdup() for symname returned NULL");
    if (lib->syms == NULL)
        lib->syms = vector_new(struct sym);
    vector_push_back(&lib->syms, new_sym);

    return sym_handle;
}

void p_librtld_close(struct p_lib **lib_p)
{
    if (lib_p == NULL || *lib_p == NULL) return;
    struct p_lib *lib = *lib_p;

    if (lib->handle != NULL) {
        if (false && dlclose(lib->handle)) {
            s_log_error("Failed to dlclose \"%s\": %s",
                lib->so_name ? lib->so_name : "<N/A>", dlerror());
        }
        lib->handle = NULL;
    }
    if (lib->so_name != NULL) {
        u_nfree(&lib->so_name);
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
