#include <core/int.h>
#include <core/librtld.h>
#include <string.h>
#include <pthread.h>
#define P_INTERNAL_GUARD__
#include "libx11-rtld.h"
#undef P_INTERNAL_GUARD__

static struct lib *X11_lib = NULL;
static pthread_mutex_t X11_lib_mutex = PTHREAD_MUTEX_INITIALIZER;

i32 libX11_load(struct libX11 *o)
{
    if (o == NULL) return 1;

    memset(o, 0, sizeof(struct libX11));

    /* I used goto instead of if(X11_lib == NULL) { ... }
     * to avoid nesting the code below - for better readability */
    if (X11_lib != NULL) goto copy_sym_handles;

    static const char *sym_names[] = {
#define X_(ret_type, name, ...) #name,
        X11_SYM_LIST
#undef X_
        NULL
    };

    pthread_mutex_lock(&X11_lib_mutex);
    X11_lib = librtld_load(X11_LIB_NAME, sym_names);
    pthread_mutex_unlock(&X11_lib_mutex);
    if (X11_lib == NULL)
        return 1;
    
copy_sym_handles:
#define X_(ret_type, name, ...) o->name = librtld_get_sym_handle(X11_lib, #name);
    X11_SYM_LIST
#undef X_

    return 0;
}

bool libX11_is_loaded()
{
    return X11_lib != NULL;
}

void libX11_unload()
{
    if (X11_lib == NULL) return;

    pthread_mutex_lock(&X11_lib_mutex);
    librtld_close(X11_lib);
    X11_lib = NULL;
    pthread_mutex_unlock(&X11_lib_mutex);
}
