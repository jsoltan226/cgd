#ifndef P_LIBRTLD_H_
#define P_LIBRTLD_H_

#include <core/int.h>

struct p_sym;
typedef struct p_sym p_sym_t;

struct p_lib;

struct p_lib * p_librtld_load(const char *libname, const char **symnames);
void p_librtld_close(struct p_lib **lib_p);
void * p_librtld_get_sym_handle(struct p_lib *lib, const char *symname);

#endif /* LIBRTLD_H_ */
