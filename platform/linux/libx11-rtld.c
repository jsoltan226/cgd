#include <core/int.h>
#include <core/librtld.h>
#include <string.h>
#include <pthread.h>
#define P_INTERNAL_GUARD__
#include "libx11-rtld.h"
#undef P_INTERNAL_GUARD__

static struct lib *X11_lib = NULL;
static pthread_mutex_t X11_lib_mutex = PTHREAD_MUTEX_INITIALIZER;

static struct lib *XExt_lib = NULL;
static pthread_mutex_t XExt_lib_mutex = PTHREAD_MUTEX_INITIALIZER;

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

void libX11_unload()
{
    if (X11_lib == NULL) return;

    pthread_mutex_lock(&X11_lib_mutex);
    librtld_close(X11_lib);
    X11_lib = NULL;
    pthread_mutex_unlock(&X11_lib_mutex);
}

i32 libXExt_load(struct libXExt *o)
{
    if (o == NULL) return 1;

    memset(o, 0, sizeof(struct libXExt));

    /* I used goto instead of if(XExt_lib == NULL) { ... }
     * to avoid nesting the code below - for better readability */
    if (XExt_lib != NULL) goto copy_sym_handles;

    static const char *sym_names[] = {
#define X_(ret_type, name, ...) #name,
        XEXT_SYM_LIST
#undef X_
        NULL
    };

    pthread_mutex_lock(&XExt_lib_mutex);
    XExt_lib = librtld_load(XEXT_LIB_NAME, sym_names);
    pthread_mutex_unlock(&XExt_lib_mutex);
    if (XExt_lib == NULL)
        return 1;
    
copy_sym_handles:
#define X_(ret_type, name, ...) o->name = librtld_get_sym_handle(XExt_lib, #name);
    XEXT_SYM_LIST
#undef X_

    return 0;
}

void libXExt_unload()
{
    if (XExt_lib == NULL) return;

    pthread_mutex_lock(&XExt_lib_mutex);
    librtld_close(XExt_lib);
    XExt_lib = NULL;
    pthread_mutex_unlock(&XExt_lib_mutex);
}
