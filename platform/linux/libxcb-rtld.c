#include <core/log.h>
#include <core/int.h>
#include <core/util.h>
#include <core/librtld.h>
#include <core/datastruct/vector.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>
#define P_INTERNAL_GUARD__
#include "libxcb-rtld.h"
#undef P_INTERNAL_GUARD__

#define MODULE_NAME "libxcb-rtld"

#define ERROR_MAGIC -1

static struct lib *g_libxcb_lib = NULL;
static struct lib *g_libxcb_image_lib = NULL;
static struct lib *g_libxcb_icccm_lib = NULL;

static struct libxcb g_libxcb_syms = { 0 };
static i32 g_n_active_handles = 0;
static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;

#define X_(ret_type, name, ...) #name,
static const char *libxcb_sym_list[] = {
    LIBXCB_SYM_LIST
    NULL
};

static const char *libxcb_image_sym_list[] = {
    LIBXCB_IMAGE_SYM_LIST
    NULL
};

static const char *libxcb_icccm_sym_list[] = {
    LIBXCB_ICCCM_SYM_LIST
    NULL
};
#undef X_

i32 libxcb_load(struct libxcb *o)
{
    i32 ret = 0;
    u_check_params(o != NULL);
    memset(o, 0, sizeof(struct libxcb));

    pthread_mutex_lock(&g_mutex);

    if (g_n_active_handles == ERROR_MAGIC) {
        ret = -1;
        goto end;
    }

    if (g_libxcb_lib == NULL ||
        g_libxcb_image_lib == NULL ||
        g_libxcb_icccm_lib == NULL
    ) {
        g_n_active_handles = 0;
        memset(&g_libxcb_syms, 0, sizeof(struct libxcb));

        *(i32 *)&g_libxcb_syms.handleno_ = -1; /* Cast away const */
        *(bool *)&g_libxcb_syms.failed_ = false; /* Cast away const */

        g_libxcb_lib = librtld_load(LIBXCB_SO_NAME, libxcb_sym_list);

        g_libxcb_image_lib = librtld_load(LIBXCB_IMAGE_SO_NAME,
                libxcb_image_sym_list);

        g_libxcb_icccm_lib = librtld_load(LIBXCB_ICCCM_SO_NAME,
                libxcb_icccm_sym_list);

        if (g_libxcb_lib == NULL ||
            g_libxcb_image_lib == NULL ||
            g_libxcb_icccm_lib == NULL
        ) {
            g_n_active_handles = ERROR_MAGIC;
            ret = 1;
            goto end;
        }

#define X_(ret_type, name, ...) g_libxcb_syms.name = \
                librtld_get_sym_handle(g_libxcb_lib, #name);
            LIBXCB_SYM_LIST
#undef X_
#define X_(ret_type, name, ...) g_libxcb_syms.name = \
                librtld_get_sym_handle(g_libxcb_image_lib, #name);
            LIBXCB_IMAGE_SYM_LIST
#undef X_
#define X_(ret_type, name, ...) g_libxcb_syms.name = \
                librtld_get_sym_handle(g_libxcb_icccm_lib, #name);
            LIBXCB_ICCCM_SYM_LIST
#undef X_
    }

    memcpy(o, &g_libxcb_syms, sizeof(struct libxcb));
    *(i32 *)&o->handleno_ = g_n_active_handles; /* Cast away const */
    g_n_active_handles++;

end:
    pthread_mutex_unlock(&g_mutex);
    *(bool *)&o->failed_ = ret != 0; /* Cast away const */
    return ret;
}

void libxcb_unload(struct libxcb *xcb)
{
    if (xcb == NULL || xcb->failed_) return;
    pthread_mutex_lock(&g_mutex);

    if (g_n_active_handles == ERROR_MAGIC) goto end;
    else if (g_n_active_handles <= 0) goto end;

    /* Sanity check */
    if (xcb->handleno_ >= g_n_active_handles) {
        s_log_error("%s: Invalid libxcb handle. Not unloading!", __func__);
        goto end;
    }

    if (--g_n_active_handles == 0) {
        librtld_close(g_libxcb_lib);
        librtld_close(g_libxcb_image_lib);
        librtld_close(g_libxcb_icccm_lib);
        g_libxcb_lib = NULL;
        g_libxcb_image_lib = NULL;
        g_libxcb_icccm_lib = NULL;

        memset(&g_libxcb_syms, 0, sizeof(struct libxcb));
    }

end:
    pthread_mutex_unlock(&g_mutex);
}
