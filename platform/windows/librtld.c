#include "../librtld.h"

struct p_sym {
};

struct p_lib {
};

struct p_lib * p_librtld_load(const char *libname, const char **symnames)
{
}

void p_librtld_close(struct p_lib **lib_p)
{
}

void * p_librtld_get_sym_handle(struct p_lib *lib, const char *symname)
{
}
