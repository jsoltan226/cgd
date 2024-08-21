#ifndef LIBRTLD_H_
#define LIBRTLD_H_

#include "int.h"

typedef struct sym_t {
    const char *name;
    void *handle;
} sym_t;

struct lib {
    void *handle;
    sym_t *syms;
    u32 n_syms;
};

struct lib * librtld_load(const char *libname, const char **symnames);
void librtld_close(struct lib *lib);
void * librtld_get_sym_handle(struct lib *lib, const char *symname);

#endif /* LIBRTLD_H_ */
